#!/usr/bin/env python3

import json
import sys
import logging
import copy
from pathlib import Path
from typing import Dict, Any, List

class EmMerger:

    def __init__(self, platform_manifest: Path, output_manifest: Path, app_manifests: List[Path]):
        self.logger = logging.getLogger(__name__)
        logging.basicConfig(
            level=logging.INFO,
            format='- [PROCESS MANIFEST][%(levelname)s]: %(message)s'
        )

        self.platform_manifest = platform_manifest
        self.output_manifest = output_manifest
        self.app_manifests = app_manifests

    def load_json(self, path: Path) -> Dict[str, Any]:
        try:
            with path.open("r", encoding="utf-8") as handle:
                return json.load(handle)
        except Exception as e:
            self.logger.error(f"Failed to load JSON from {path}: {e}")
            raise

    def add_unique_by_short_name(self, items: List[Dict[str, Any]],
                                 incoming_items: List[Dict[str, Any]], category: str) -> None:
        index = {item.get("shortName"): item for item in items}
        for incoming in incoming_items:
            short_name = incoming.get("shortName")
            if not short_name:
                self.logger.error(f"{category} entry is missing shortName: {incoming}")
                raise ValueError(f"{category} entry is missing shortName")
            existing = index.get(short_name)
            if existing is not None and existing != incoming:
                self.logger.error(f"Conflicting {category} definition for '{short_name}': existing={existing}, incoming={incoming}")
                raise ValueError(f"Conflicting {category} definition for '{short_name}'")
            if existing is None:
                items.append(incoming)
                index[short_name] = incoming

    def add_processes(self, build_root: Path, processes: List[Dict[str, Any]],
                        incoming_processes: List[Dict[str, Any]], source_path: Path) -> None:
        index = {item.get("shortName"): item for item in processes}
        for process in incoming_processes:
            short_name = process.get("shortName")
            executable_name = process.get("executableName")
            if not short_name or not executable_name:
                self.logger.error(f"Process entry in {source_path} is missing required fields: {process}")
                raise ValueError(f"Process entry in {source_path} is missing required fields")

            executable_path = source_path.parent.parent / executable_name
            if executable_path.is_relative_to(build_root):
                process["executablePath"] = executable_path.relative_to(build_root).as_posix()
            else:
                process["executablePath"] = str(executable_path)

            existing = index.get(short_name)
            if existing is not None and existing != process:
                self.logger.error(f"Conflicting process definition for '{short_name}': existing={existing}, incoming={process}")
                raise ValueError(f"Conflicting process definition for '{short_name}'")
            if existing is None:
                processes.append(process)
                index[short_name] = process

    def validate_startup_config_references(self, execution_manifest: Dict[str, Any]) -> None:
        available_startup_configs = {
            item.get("shortName")
            for item in execution_manifest.get("startupConfigs", [])
            if item.get("shortName")
        }

        for process in execution_manifest.get("processes", []):
            for startup in process.get("stateDependentStartupConfigs", []):
                startup_config_ref = startup.get("startupConfigRef")
                if not startup_config_ref:
                    continue

                startup_config_name = startup_config_ref.rsplit("/", 1)[-1]
                if startup_config_name not in available_startup_configs:
                    process_name = process.get("shortName", "<unknown-process>")
                    self.logger.error(f"Process '{process_name}' references missing StartupConfig '{startup_config_name}'")
                    raise ValueError(
                        f"Process '{process_name}' references missing StartupConfig '{startup_config_name}'"
                    )

    def merge_execution_manifests(self) -> None:
        platform_root = self.load_json(self.platform_manifest)
        merged = copy.deepcopy(platform_root)
        execution_manifest = merged.setdefault("executionManifest", {})
        execution_manifest.setdefault("processes", [])
        execution_manifest.setdefault("executables", [])
        execution_manifest.setdefault("startupConfigs", [])

        platform_execution_manifest = execution_manifest
        self.add_unique_by_short_name(
            execution_manifest["executables"],
            platform_execution_manifest.get("executables", []),
            "executable",
        )
        self.add_unique_by_short_name(
            execution_manifest["startupConfigs"],
            platform_execution_manifest.get("startupConfigs", []),
            "startupConfig",
        )

        build_root = self.output_manifest.parent.parent
        for app_manifest in self.app_manifests:
            app_root = self.load_json(app_manifest)
            app_execution_manifest = app_root.get("executionManifest", {})
            self.add_processes(
                build_root,
                execution_manifest["processes"],
                app_execution_manifest.get("processes", []),
                app_manifest,
            )
            self.add_unique_by_short_name(
                execution_manifest["executables"],
                app_execution_manifest.get("executables", []),
                "executable",
            )
            self.add_unique_by_short_name(
                execution_manifest["startupConfigs"],
                app_execution_manifest.get("startupConfigs", []),
                "startupConfig",
            )

        self.validate_startup_config_references(execution_manifest)

        self.output_manifest.parent.mkdir(parents=True, exist_ok=True)
        with self.output_manifest.open("w", encoding="utf-8") as handle:
            json.dump(merged, handle, indent=2, ensure_ascii=False)

def main() -> int:
    if len(sys.argv) < 3:
        print("Usage: python merge_execution_manifests.py <platform_json> <output_json> [app_json ...]")
        return 1

    platform_manifest = Path(sys.argv[1])
    output_manifest = Path(sys.argv[2])
    app_manifests = [Path(argument) for argument in sys.argv[3:]]

    em_merger = EmMerger(platform_manifest, output_manifest, app_manifests)

    try:
        em_merger.merge_execution_manifests()
        if app_manifests:
            em_merger.logger.info(f"Successfully merged following app_execution_manifests into {output_manifest}:\n    - {'\n    - '.join(str(app) for app in app_manifests)}")
        else:
            em_merger.logger.info(f"No app_execution_manifests provided, copying platform manifest to {output_manifest}")
    except Exception as e:
        em_merger.logger.error(f"Failed to merge execution manifests: {e}")
        return 1


    return 0


if __name__ == "__main__":
    sys.exit(main())
