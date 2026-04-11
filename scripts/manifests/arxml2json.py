#!/usr/bin/env python3
"""
ARXML to JSON converter for Adaptive AUTOSAR manifests.

The generated JSON is intentionally structured around AUTOSAR concepts instead of
legacy project-specific shortcuts so that Execution Management can keep machine
configuration separate from per-process execution configuration.
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
    
    def __init__(self):
        """Initialize the converter with logging setup."""
        self.logger = logging.getLogger(__name__)
        self.manifest_type = None
        self.namespace = None  # Will be detected from document
        self.input_file = ""

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
            self.input_file = input_file
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
            elif self.manifest_type == 'service_instance':
                json_data = self._convert_service_instance_manifest(root)
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

        input_name = Path(self.input_file).name
        if "service_instance_manifest" in input_name:
            self.manifest_type = 'service_instance'
            return
        if "execution_manifest" in input_name:
            self.manifest_type = 'execution'
            return

        if root.find('.//ar:MACHINE', ns) is not None:
            self.manifest_type = 'machine'
        elif root.find('.//ar:PROCESS', ns) is not None:
            self.manifest_type = 'execution'
        elif root.find('.//ar:STARTUP-CONFIG', ns) is not None or root.find('.//ar:EXECUTABLE', ns) is not None:
            self.manifest_type = 'execution'
        else:
            self.manifest_type = 'generic'

    def _convert_service_instance_manifest(self, root: ET.Element) -> Dict[str, Any]:
        ns = self._make_namespace_map()

        service_deployment = root.find('.//ar:USER-DEFINED-SERVICE-INTERFACE-DEPLOYMENT', ns)
        provided_instance = root.find('.//ar:PROVIDED-USER-DEFINED-SERVICE-INSTANCE', ns)
        required_instance = root.find('.//ar:REQUIRED-USER-DEFINED-SERVICE-INSTANCE', ns)

        if service_deployment is None:
            return {'serviceInstanceManifest': {}}

        instance_element = provided_instance if provided_instance is not None else required_instance
        request_response_deployment = None
        fire_and_forget_deployment = None

        for method_deployment in service_deployment.findall(
                './ar:METHOD-DEPLOYMENTS/ar:USER-DEFINED-METHOD-DEPLOYMENT', ns):
            communication_mode = self._get_admin_sd(
                method_deployment, 'OPENAA', 'COMMUNICATION-MODE')
            if communication_mode == 'REQUEST-RESPONSE':
                request_response_deployment = method_deployment
            elif communication_mode == 'FIRE-AND-FORGET':
                fire_and_forget_deployment = method_deployment

        event_deployment = service_deployment.find(
            './ar:EVENT-DEPLOYMENTS/ar:USER-DEFINED-EVENT-DEPLOYMENT', ns)

        service_interface_ref = self._get_text(service_deployment, 'ar:SERVICE-INTERFACE-REF')
        event_ref = self._get_text(event_deployment, 'ar:EVENT-REF') if event_deployment is not None else ''
        request_response_ref = self._get_text(
            request_response_deployment, 'ar:METHOD-REF') if request_response_deployment is not None else ''
        fire_and_forget_ref = self._get_text(
            fire_and_forget_deployment, 'ar:METHOD-REF') if fire_and_forget_deployment is not None else ''

        return {
            'serviceInstanceManifest': {
                'shortName': self._get_text(service_deployment, 'ar:SHORT-NAME'),
                'serviceInterfaceRef': service_interface_ref,
                'serviceIdentifier': self._get_admin_sd(
                    service_deployment, 'OPENAA', 'SERVICE-IDENTIFIER'),
                'transportBinding': self._get_admin_sd(
                    service_deployment, 'OPENAA', 'TRANSPORT-BINDING'),
                'instanceSpecifier': self._get_admin_sd(
                    instance_element, 'OPENAA', 'INSTANCE-SPECIFIER') if instance_element is not None else '',
                'instanceIdentifier': self._get_admin_sd(
                    instance_element, 'OPENAA', 'INSTANCE-IDENTIFIER') if instance_element is not None else '',
                'eventName': event_ref.split('/')[-1] if event_ref else self._get_text(
                    event_deployment, 'ar:SHORT-NAME') if event_deployment is not None else '',
                'eventChannel': self._get_admin_sd(
                    event_deployment, 'OPENAA', 'CHANNEL') if event_deployment is not None else '',
                'methodName': request_response_ref.split('/')[-1] if request_response_ref else self._get_text(
                    request_response_deployment, 'ar:SHORT-NAME') if request_response_deployment is not None else '',
                'methodChannel': self._get_admin_sd(
                    request_response_deployment, 'OPENAA', 'CHANNEL') if request_response_deployment is not None else '',
                'fireAndForgetName': fire_and_forget_ref.split('/')[-1] if fire_and_forget_ref else self._get_text(
                    fire_and_forget_deployment, 'ar:SHORT-NAME') if fire_and_forget_deployment is not None else '',
                'fireAndForgetChannel': self._get_admin_sd(
                    fire_and_forget_deployment, 'OPENAA', 'CHANNEL') if fire_and_forget_deployment is not None else '',
            }
        }
    
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

        machine = root.find('.//ar:MACHINE', ns)
        if machine is not None:
            machine_obj = {}
            machine_obj['shortName'] = self._get_text(machine, 'ar:SHORT-NAME')

            timeout_elem = machine.find('ar:DEFAULT-APPLICATION-TIMEOUT', ns)
            if timeout_elem is not None:
                machine_obj['defaultApplicationTimeout'] = {
                    'enterTimeoutValue': self._get_int(timeout_elem, 'ar:ENTER-TIMEOUT-VALUE'),
                    'exitTimeoutValue': self._get_int(timeout_elem, 'ar:EXIT-TIMEOUT-VALUE')
                }

            machine_obj['functionGroups'] = self._extract_function_groups(root)
            machine_obj['modules'] = self._extract_modules(machine)

            tpelb = self._get_text(machine, 'ar:TRUSTED-PLATFORM-EXECUTABLE-LAUNCH-BEHAVIOR')
            if tpelb:
                machine_obj['trustedPlatformExecutableLaunchBehavior'] = tpelb

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

        execution_data = {
            'executionManifest': {
                'processes': self._extract_processes(root),
                'executables': self._extract_executables(root),
                'startupConfigs': self._extract_startup_configs(root),
            }
        }

        return execution_data

    def _extract_processes(self, root: ET.Element) -> List[Dict[str, Any]]:
        """Extract all process definitions from the execution manifest."""
        ns = self._make_namespace_map()
        processes = []

        for process in root.findall('.//ar:PROCESS', ns):
            process_obj = {
                'shortName': self._get_text(process, 'ar:SHORT-NAME'),
                'stateDependentStartupConfigs': self._extract_state_dependent_configs(root, process),
            }

            executable_ref = self._get_first_text(process, [
                'ar:EXECUTABLE-REFS/ar:EXECUTABLE-REF',
                'ar:EXECUTABLE-REF'
            ])
            if executable_ref:
                process_obj['executableRef'] = executable_ref
                process_obj['executableName'] = executable_ref.split('/')[-1]

            restart_attempts = self._get_int(process, 'ar:NUMBER-OF-RESTART-ATTEMPTS')
            if restart_attempts is not None:
                process_obj['numberOfRestartAttempts'] = restart_attempts

            state_machine = process.find('ar:PROCESS-STATE-MACHINE', ns)
            if state_machine is not None:
                process_obj['processStateMachine'] = {
                    'shortName': self._get_text(state_machine, 'ar:SHORT-NAME'),
                    'typeRef': self._get_text(state_machine, 'ar:TYPE-TREF'),
                }

            if process_obj['shortName']:
                processes.append(process_obj)

        return processes
    
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
        mode_groups = {
            self._get_text(mode_group, 'ar:SHORT-NAME'): mode_group
            for mode_group in root.findall('.//ar:MODE-DECLARATION-GROUP', ns)
        }

        for prototype in root.findall('.//ar:MODE-DECLARATION-GROUP-PROTOTYPE', ns):
            prototype_name = self._get_text(prototype, 'ar:SHORT-NAME')
            type_ref = self._get_text(prototype, 'ar:TYPE-TREF')
            type_name = type_ref.split('/')[-1] if type_ref else ''
            mode_group = mode_groups.get(type_name)
            if not prototype_name or mode_group is None:
                continue

            fg_obj = {
                'name': prototype_name,
                'typeRef': type_ref,
                'states': [],
                'initialState': None
            }

            for mode in mode_group.findall('.//ar:MODE-DECLARATION', ns):
                mode_name = self._get_text(mode, 'ar:SHORT-NAME')
                if mode_name:
                    fg_obj['states'].append(mode_name)

            initial_mode_ref = mode_group.find('ar:INITIAL-MODE-REF', ns)
            if initial_mode_ref is not None and initial_mode_ref.text:
                fg_obj['initialState'] = initial_mode_ref.text.split('/')[-1]

            if fg_obj['states']:
                function_groups.append(fg_obj)

        return function_groups
    
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
        arg_elem = parent.find('ar:PROCESS-ARGUMENTS', ns)
        if arg_elem is None:
            return arguments

        for argument in arg_elem.findall('ar:PROCESS-ARGUMENT', ns):
            value = self._get_text(argument, 'ar:ARGUMENT')
            if value:
                arguments.append(value)

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
        env_elem = parent.find('ar:ENVIRONMENT-VARIABLES', ns)
        if env_elem is not None:
            vars_list = env_elem.findall('ar:TAG-WITH-OPTIONAL-VALUE', ns)
            for var in vars_list:
                var_name = self._get_text(var, 'ar:KEY')
                var_value = self._get_text(var, 'ar:VALUE')
                if var_name:
                    env_vars.append({'key': var_name, 'value': var_value or ''})
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
        
        state_configs_container = None
        if process_elem is not None:
            state_configs_container = process_elem.find('ar:STATE-DEPENDENT-STARTUP-CONFIGS', ns)
        if state_configs_container is None:
            state_configs_container = root.find('.//ar:STATE-DEPENDENT-STARTUP-CONFIGS', ns)

        if state_configs_container is not None:
            state_configs = state_configs_container.findall('ar:STATE-DEPENDENT-STARTUP-CONFIG', ns)

            for config in state_configs:
                config_obj = {}
                config_obj['functionGroupStates'] = []

                fg_state_irefs = config.find('ar:FUNCTION-GROUP-STATE-IREFS', ns)
                if fg_state_irefs is not None:
                    fg_state_refs = fg_state_irefs.findall('ar:FUNCTION-GROUP-STATE-IREF', ns)
                    for fg_state_ref in fg_state_refs:
                        context_ref = self._get_text(fg_state_ref, 'ar:CONTEXT-MODE-DECLARATION-GROUP-PROTOTYPE-REF')
                        target_ref = self._get_text(fg_state_ref, 'ar:TARGET-MODE-DECLARATION-REF')

                        if context_ref and target_ref:
                            fg_obj = {
                                'functionGroup': context_ref.split('/')[-1],
                                'state': target_ref.split('/')[-1]
                            }
                            config_obj['functionGroupStates'].append(fg_obj)

                startup_config_ref = self._get_text(config, 'ar:STARTUP-CONFIG-REF')
                if startup_config_ref:
                    config_obj['startupConfigRef'] = startup_config_ref

                configs.append(config_obj)

        return configs

    def _extract_executables(self, root: ET.Element) -> List[Dict[str, Any]]:
        """Extract executable definitions referenced by the execution manifest."""
        ns = self._make_namespace_map()
        executables = []

        for executable in root.findall('.//ar:EXECUTABLE', ns):
            executable_obj = {
                'shortName': self._get_text(executable, 'ar:SHORT-NAME'),
            }

            reporting = self._get_text(executable, 'ar:REPORTING-BEHAVIOR')
            if reporting:
                executable_obj['reportingBehavior'] = reporting

            version = self._get_text(executable, 'ar:VERSION')
            if version:
                executable_obj['version'] = version

            if executable_obj['shortName']:
                executables.append(executable_obj)

        return executables

    def _extract_startup_configs(self, root: ET.Element) -> List[Dict[str, Any]]:
        """Extract reusable startup configuration objects."""
        ns = self._make_namespace_map()
        startup_configs = []

        for startup in root.findall('.//ar:STARTUP-CONFIG', ns):
            startup_obj = {
                'shortName': self._get_text(startup, 'ar:SHORT-NAME'),
                'processArguments': self._extract_arguments(startup),
                'environmentVariables': self._extract_env_vars(startup),
            }

            permission = self._get_text(startup, 'ar:PERMISSION-TO-CREATE-CHILD-PROCESS')
            if permission:
                startup_obj['permissionToCreateChildProcess'] = permission.lower() == 'true'

            scheduling_policy = self._get_text(startup, 'ar:SCHEDULING-POLICY')
            if scheduling_policy:
                startup_obj['schedulingPolicy'] = scheduling_policy

            scheduling_priority = self._get_int(startup, 'ar:SCHEDULING-PRIORITY')
            if scheduling_priority is not None:
                startup_obj['schedulingPriority'] = scheduling_priority

            termination_behavior = self._get_text(startup, 'ar:TERMINATION-BEHAVIOR')
            if termination_behavior:
                startup_obj['terminationBehavior'] = termination_behavior

            timeout = startup.find('ar:TIMEOUT', ns)
            if timeout is not None:
                startup_obj['timeout'] = {
                    'enterTimeoutValue': self._get_int(timeout, 'ar:ENTER-TIMEOUT-VALUE'),
                    'exitTimeoutValue': self._get_int(timeout, 'ar:EXIT-TIMEOUT-VALUE'),
                }

            if startup_obj['shortName']:
                startup_configs.append(startup_obj)

        return startup_configs
    
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

    def _get_first_text(self, element: ET.Element, paths: List[str], default: str = '') -> str:
        """Return the first non-empty text value among a list of XPath expressions."""
        for path in paths:
            value = self._get_text(element, path)
            if value:
                return value
        return default

    def _get_admin_sd(
            self,
            element: Optional[ET.Element],
            sdg_gid: str,
            sd_gid: str,
            default: str = '') -> str:
        """Read an SD value from ADMIN-DATA/SDGS."""
        if element is None:
            return default

        ns = self._make_namespace_map()
        path = (
            f"./ar:ADMIN-DATA/ar:SDGS/ar:SDG[@GID='{sdg_gid}']"
            f"/ar:SD[@GID='{sd_gid}']")
        return self._get_text(element, path, default)
    
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
