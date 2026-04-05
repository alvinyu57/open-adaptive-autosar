#!/usr/bin/env python3
"""
ARXML to JSON Converter for Adaptive AUTOSAR Manifests

This script converts AUTOSAR ARXML (XML-based configuration) files to JSON format.
It supports both machine manifests and execution manifests.

Usage:
    python arxml2json.py <input_arxml_file> <output_json_file>

Example:
    python arxml2json.py src/manifests/machine_manifest.arxml src/manifests/machine_manifest.json
    python arxml2json.py apps/01_hello_world/manifests/execution_manifest.arxml apps/01_hello_world/manifests/execution_manifest.json
"""

import json
import sys
import logging
from pathlib import Path
from typing import Dict, Any, Optional, List
from xml.etree import ElementTree as ET


class ARXMLConverter:
    """
    Converter for AUTOSAR ARXML files to JSON format.
    
    This class handles parsing ARXML XML structure and converting it to JSON,
    with support for different manifest types (machine, execution).
    """
    
    # Common AUTOSAR XML namespaces
    NAMESPACES = {
        'ar4': 'http://autosar.org/schema/r4.0',
        'ar25': 'http://autosar.org/schema/r25',
        'ar51': 'http://autosar.org/schema/r51'
    }
    
    def __init__(self):
        """Initialize the converter with logging setup."""
        self.logger = logging.getLogger(__name__)
        self.manifest_type = None
        self.namespace = None  # Will be detected from document

        logging.basicConfig(
            level=logging.INFO,
            format='- [PROCESS MANIFEST][%(levelname)s]: %(message)s'
        )
    
    def _get_namespace(self, root: ET.Element) -> str:
        """
        Extract the actual namespace from the root element.
        
        Args:
            root: The root element
            
        Returns:
            str: The namespace URI, or empty string if not found
        """
        if root.tag.startswith('{'):
            return root.tag.split('}')[0][1:]
        return ''
    
    def _make_namespace_map(self) -> Dict[str, str]:
        """
        Create a namespace map for the detected namespace.
        
        Returns:
            dict: Namespace mapping with 'ar' as prefix
        """
        if self.namespace:
            return {'ar': self.namespace}
        return {'ar': 'http://autosar.org/schema/r4.0'}  # Default fallback
    
    def convert(self, input_file: str, output_file: str) -> bool:
        """
        Convert an ARXML file to JSON and save it to the output file.
        
        Args:
            input_file: Path to the input ARXML file
            output_file: Path to the output JSON file
            
        Returns:
            bool: True if conversion was successful, False otherwise
        """
        try:
            # Parse the ARXML file
            tree = ET.parse(input_file)
            root = tree.getroot()
            
            # Detect namespace from document
            self.namespace = self._get_namespace(root)
            self.logger.debug(f"Detected namespace: {self.namespace}")
            
            # Detect manifest type from content
            self._detect_manifest_type(root)
            
            # Convert based on manifest type
            if self.manifest_type == 'machine':
                json_data = self._convert_machine_manifest(root)
            elif self.manifest_type == 'execution':
                json_data = self._convert_execution_manifest(root)
            else:
                # Fallback generic conversion
                json_data = self._generic_convert(root)
            
            # Write to output file
            self._write_json_file(json_data, output_file)
            self.logger.info(f"{input_file} is converted to {output_file}")
            return True
            
        except FileNotFoundError:
            self.logger.error(f"Input file not found: {input_file}")
            return False
        except ET.ParseError as e:
            self.logger.error(f"Failed to parse ARXML file: {e}")
            return False
        except Exception as e:
            self.logger.error(f"Conversion failed: {e}")
            return False
    
    def _detect_manifest_type(self, root: ET.Element) -> None:
        """
        Detect the type of manifest from ARXML structure.
        
        Args:
            root: The root element of the ARXML document
        """
        ns = self._make_namespace_map()
        # Look for characteristic elements to determine manifest type
        ar_packages = root.findall('.//ar:AR-PACKAGES/ar:AR-PACKAGE', ns)
        
        for package in ar_packages:
            short_name = self._get_text(package, 'ar:SHORT-NAME')
            
            if short_name and 'Machines' in short_name:
                self.manifest_type = 'machine'
                return
            elif short_name and 'ExecutionManifest' in short_name:
                self.manifest_type = 'execution'
                return
        
        # Check for specific elements
        if root.find('.//ar:MACHINE', ns) is not None:
            self.manifest_type = 'machine'
        elif root.find('.//ar:PROCESS', ns) is not None:
            self.manifest_type = 'execution'
        else:
            self.manifest_type = 'generic'
    
    def _convert_machine_manifest(self, root: ET.Element) -> Dict[str, Any]:
        """
        Convert a machine manifest ARXML to JSON.
        
        Args:
            root: The root element of the ARXML document
            
        Returns:
            dict: The converted JSON data structure
        """
        ns = self._make_namespace_map()
        machine_data = {}
        
        # Find the Machine element
        machine = root.find('.//ar:MACHINE', ns)
        if machine is not None:
            machine_obj = {}
            machine_obj['shortName'] = self._get_text(machine, 'ar:SHORT-NAME')
            
            # Extract default application timeouts
            timeout_elem = machine.find('ar:DEFAULT-APPLICATION-TIMEOUT', ns)
            if timeout_elem is not None:
                machine_obj['defaultTimeout'] = {
                    'enterTimeoutValue': self._get_int(timeout_elem, 'ar:ENTER-TIMEOUT-VALUE'),
                    'exitTimeoutValue': self._get_int(timeout_elem, 'ar:EXIT-TIMEOUT-VALUE')
                }
            
            # Extract function groups
            machine_obj['functionGroups'] = self._extract_function_groups(root)
            
            # Extract processes (if present in the machine manifest)
            machine_obj['processes'] = self._extract_processes(machine)
            
            # Extract module instantiations (OS and middleware)
            machine_obj['modules'] = self._extract_modules(machine)
            
            # Extract trusted platform behavior
            tpelb = self._get_text(machine, 'ar:TRUSTED-PLATFORM-EXECUTABLE-LAUNCH-BEHAVIOR')
            if tpelb:
                machine_obj['trustedPlatformBehavior'] = tpelb
            
            machine_data = {'machine': machine_obj}
        
        return machine_data
    
    def _convert_execution_manifest(self, root: ET.Element) -> Dict[str, Any]:
        """
        Convert an execution manifest ARXML to JSON.
        
        Args:
            root: The root element of the ARXML document
            
        Returns:
            dict: The converted JSON data structure
        """
        ns = self._make_namespace_map()
        execution_data = {}
        
        # Find the PROCESS element
        process = root.find('.//ar:PROCESS', ns)
        if process is not None:
            process_obj = {}
            process_obj['shortName'] = self._get_text(process, 'ar:SHORT-NAME')
            
            # Extract executable reference
            executable_ref = self._get_text(process, 'ar:EXECUTABLE-REF')
            if executable_ref:
                # Extract just the executable name from the reference path
                process_obj['executable'] = executable_ref.split('/')[-1]
            
            # Find the EXECUTABLE element to get reporting behavior
            executable = root.find('.//ar:EXECUTABLE', ns)
            if executable is not None:
                reporting = self._get_text(executable, 'ar:REPORTING-BEHAVIOR')
                if reporting:
                    # Convert AUTOSAR naming to camelCase
                    if reporting == 'REPORTS-EXECUTION-STATE':
                        process_obj['reportingBehavior'] = 'reportsExecutionState'
                    else:
                        process_obj['reportingBehavior'] = reporting
            
            # Extract arguments and environment variables
            process_obj['arguments'] = self._extract_arguments(process)
            process_obj['environmentVariables'] = self._extract_env_vars(process)
            
            # Extract state dependent startup configs
            process_obj['stateDependentStartupConfigs'] = self._extract_state_dependent_configs(root, process)
            
            execution_data = {'executionManifest': {'process': process_obj}}
        
        return execution_data
    
    def _extract_function_groups(self, root: ET.Element) -> List[Dict[str, Any]]:
        """
        Extract function groups and their states from the manifest.
        
        Args:
            root: The root element of the ARXML document
            
        Returns:
            list: List of function group definitions with their states
        """
        ns = self._make_namespace_map()
        function_groups = []
        
        # Find Mode Declaration Groups which define states
        mode_groups = root.findall('.//ar:MODE-DECLARATION-GROUP', ns)
        for mode_group in mode_groups:
            group_name = self._get_text(mode_group, 'ar:SHORT-NAME')
            if group_name:
                fg_obj = {
                    'name': group_name,
                    'states': [],
                    'initialState': None
                }
                
                # Extract states
                modes = mode_group.findall('.//ar:MODE-DECLARATION', ns)
                for mode in modes:
                    mode_name = self._get_text(mode, 'ar:SHORT-NAME')
                    if mode_name:
                        fg_obj['states'].append(mode_name)
                
                # Extract initial mode
                initial_mode_ref = mode_group.find('ar:INITIAL-MODE-REF', ns)
                if initial_mode_ref is not None and initial_mode_ref.text:
                    # Extract the last part of the reference path
                    ref_path = initial_mode_ref.text
                    fg_obj['initialState'] = ref_path.split('/')[-1]
                
                if fg_obj['states']:
                    function_groups.append(fg_obj)
        
        return function_groups
    
    def _extract_processes(self, parent: ET.Element) -> List[Dict[str, Any]]:
        """
        Extract process definitions.
        
        Args:
            parent: The parent element to search within
            
        Returns:
            list: List of process definitions
        """
        processes = []
        # Additional process extraction logic can be added here
        return processes
    
    def _extract_modules(self, parent: ET.Element) -> List[Dict[str, Any]]:
        """
        Extract module instantiations (OS, middleware).
        
        Args:
            parent: The parent element to search within
            
        Returns:
            list: List of module definitions
        """
        ns = self._make_namespace_map()
        modules = []
        
        # Find MODULE-INSTANTIATIONS container
        module_insts_container = parent.find('ar:MODULE-INSTANTIATIONS', ns)
        if module_insts_container is None:
            return modules
        
        # Extract all types of module instantiations
        for module in module_insts_container:
            module_obj = {}
            # Get element type from tag (e.g., OS-MODULE-INSTANTIATION)
            module_obj['type'] = module.tag.split('}')[-1]
            module_obj['shortName'] = self._get_text(module, 'ar:SHORT-NAME')
            
            # Extract resource groups
            resource_groups_container = module.find('ar:RESOURCE-GROUPS', ns)
            if resource_groups_container is not None:
                resource_groups = resource_groups_container.findall('ar:RESOURCE-GROUP', ns)
                if resource_groups:
                    module_obj['resourceGroups'] = []
                    for rg in resource_groups:
                        rg_name = self._get_text(rg, 'ar:SHORT-NAME')
                        if rg_name:
                            module_obj['resourceGroups'].append({'name': rg_name})
            
            modules.append(module_obj)
        
        return modules
    
    def _extract_arguments(self, parent: ET.Element) -> List[str]:
        """
        Extract command line arguments.
        
        Args:
            parent: The parent element to search within
            
        Returns:
            list: List of arguments
        """
        ns = self._make_namespace_map()
        arguments = []
        # Extract from specific elements if present
        arg_elem = parent.find('.//ar:ARGUMENTS', ns)
        if arg_elem is not None:
            args = arg_elem.findall('ar:TEXT', ns)
            arguments = [self._get_text(arg, '.') for arg in args]
        return arguments
    
    def _extract_env_vars(self, parent: ET.Element) -> List[Dict[str, str]]:
        """
        Extract environment variables.
        
        Args:
            parent: The parent element to search within
            
        Returns:
            list: List of environment variable definitions
        """
        ns = self._make_namespace_map()
        env_vars = []
        # Extract from specific elements if present
        env_elem = parent.find('.//ar:ENVIRONMENT-VARIABLES', ns)
        if env_elem is not None:
            vars_list = env_elem.findall('ar:VARIABLE', ns)
            for var in vars_list:
                var_name = self._get_text(var, 'ar:NAME')
                var_value = self._get_text(var, 'ar:VALUE')
                if var_name:
                    env_vars.append({'name': var_name, 'value': var_value or ''})
        return env_vars
    
    def _extract_state_dependent_configs(self, root: ET.Element, process_elem: Optional[ET.Element] = None) -> List[Dict[str, Any]]:
        """
        Extract state-dependent startup configurations.
        
        Args:
            root: The root element of the ARXML document
            process_elem: Optional process element for additional context
            
        Returns:
            list: List of state-dependent startup configurations
        """
        ns = self._make_namespace_map()
        configs = []
        
        # Find the STATE-DEPENDENT-STARTUP-CONFIGS container
        state_configs_container = root.find('.//ar:STATE-DEPENDENT-STARTUP-CONFIGS', ns)
        if state_configs_container is None and process_elem is not None:
            state_configs_container = process_elem.find('ar:STATE-DEPENDENT-STARTUP-CONFIGS', ns)
        
        if state_configs_container is not None:
            state_configs = state_configs_container.findall('ar:STATE-DEPENDENT-STARTUP-CONFIG', ns)
            
            for config in state_configs:
                config_obj = {}
                config_obj['functionGroupStates'] = []
                
                # Extract function group state references
                fg_state_irefs = config.find('ar:FUNCTION-GROUP-STATE-IREFS', ns)
                if fg_state_irefs is not None:
                    fg_state_refs = fg_state_irefs.findall('.//ar:FUNCTION-GROUP-STATE-IN-FUNCTION-GROUP-SET-INSTANCE-REF', ns)
                    for fg_state_ref in fg_state_refs:
                        context_ref = self._get_text(fg_state_ref, 'ar:CONTEXT-MODE-DECLARATION-GROUP-PROTOTYPE-REF')
                        target_ref = self._get_text(fg_state_ref, 'ar:TARGET-MODE-DECLARATION-REF')
                        
                        if context_ref and target_ref:
                            fg_obj = {
                                'functionGroup': context_ref.split('/')[-1],
                                'state': target_ref.split('/')[-1]
                            }
                            config_obj['functionGroupStates'].append(fg_obj)
                
                # Extract startup configuration reference
                startup_config_ref = self._get_text(config, 'ar:STARTUP-CONFIG-REF')
                
                # Find the referenced STARTUP-CONFIG element
                startup_config_obj = {
                    'arguments': [],
                    'environmentVariables': [],
                    'terminationBehavior': 'processIsSelfTerminating'
                }
                
                if startup_config_ref:
                    # Search for the startup config by reference
                    startup_configs = root.findall('.//ar:STARTUP-CONFIG', ns)
                    for startup in startup_configs:
                        if startup_config_ref.endswith(self._get_text(startup, 'ar:SHORT-NAME')):
                            startup_config_obj['arguments'] = self._extract_arguments(startup)
                            startup_config_obj['environmentVariables'] = self._extract_env_vars(startup)
                            termination = self._get_text(startup, 'ar:TERMINATION-BEHAVIOR')
                            if termination:
                                startup_config_obj['terminationBehavior'] = termination
                            break
                
                config_obj['startupConfig'] = startup_config_obj
                configs.append(config_obj)
        
        # If no specific configs found, create a default one
        if not configs:
            configs = [{
                'functionGroupStates': [],
                'startupConfig': {
                    'arguments': [],
                    'environmentVariables': [],
                    'terminationBehavior': 'processIsSelfTerminating'
                }
            }]
        
        return configs
    
    def _generic_convert(self, root: ET.Element) -> Dict[str, Any]:
        """
        Generic conversion for unknown manifest types.
        
        Args:
            root: The root element of the ARXML document
            
        Returns:
            dict: The converted JSON data structure
        """
        return self._element_to_dict(root)
    
    def _element_to_dict(self, element: ET.Element) -> Dict[str, Any]:
        """
        Recursively convert an XML element to a dictionary.
        
        Args:
            element: The XML element to convert
            
        Returns:
            dict: Dictionary representation of the element
        """
        result = {}
        
        # Remove namespace from tag
        tag = element.tag.split('}')[-1] if '}' in element.tag else element.tag
        
        # Add element text if present
        text = element.text.strip() if element.text else None
        
        # Process attributes
        if element.attrib:
            result['@attributes'] = element.attrib
        
        # Process children
        children = {}
        for child in element:
            child_tag = child.tag.split('}')[-1] if '}' in child.tag else child.tag
            child_dict = self._element_to_dict(child)
            
            if child_tag in children:
                # Convert to list if multiple elements with same tag
                if not isinstance(children[child_tag], list):
                    children[child_tag] = [children[child_tag]]
                children[child_tag].append(child_dict)
            else:
                children[child_tag] = child_dict
        
        if children:
            result.update(children)
        elif text:
            return text
        
        return result if result else text
    
    def _get_text(self, element: ET.Element, path: str = '.', default: str = '') -> str:
        """
        Get text content from an element using XPath.
        
        Args:
            element: The element to search within
            path: XPath expression (default is '.' for current element)
            default: Default value if not found
            
        Returns:
            str: The text content or default value
        """
        if path == '.':
            return element.text.strip() if element.text else default
        
        ns = self._make_namespace_map()
        found = element.find(path, ns)
        if found is not None and found.text:
            return found.text.strip()
        return default
    
    def _get_int(self, element: ET.Element, path: str) -> Optional[int]:
        """
        Get integer value from an element.
        
        Args:
            element: The element to search within
            path: XPath expression
            
        Returns:
            int or None: The integer value or None if not found
        """
        text = self._get_text(element, path)
        try:
            return int(text) if text else None
        except ValueError:
            return None
    
    def _write_json_file(self, data: Dict[str, Any], output_file: str) -> None:
        """
        Write JSON data to a file.
        
        Args:
            data: The data to write
            output_file: Path to the output JSON file
        """
        output_path = Path(output_file)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)


def main():
    """
    Main entry point for the ARXML to JSON converter.
    """
    if len(sys.argv) < 3:
        print("Usage: python arxml2json.py <input_arxml> <output_json>")
        print("\nExample:")
        print("  python arxml2json.py src/manifests/machine_manifest.arxml src/manifests/machine_manifest.json")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    converter = ARXMLConverter()
    success = converter.convert(input_file, output_file)
    
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
