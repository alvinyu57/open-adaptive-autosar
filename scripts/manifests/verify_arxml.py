from lxml import etree
import logging
import sys

class ArxmlValidator:
    def __init__(self, xsd_file):
        self.schema = etree.XMLSchema(etree.parse(xsd_file))
        self.logger = logging.getLogger(__name__)
        logging.basicConfig(
            level=logging.INFO,
            format='- [PROCESS MANIFEST][%(levelname)s]: %(message)s'
        )

    def validate(self, xml_file):
        doc = etree.parse(xml_file)

        try:
            self.schema.assertValid(doc)
            self.logger.info(f"{xml_file} is valid according to the schema")

        except etree.DocumentInvalid as e:
            self.logger.error(f"{xml_file} is NOT valid according to the schema:")
            self.logger.error(e)

def main():
    if len(sys.argv) != 3:
        print("Usage: python verify_arxml.py input.arxml input.xsd")
        sys.exit(1)

    xml_file = sys.argv[1]
    xsd_file = sys.argv[2]

    validator = ArxmlValidator(xsd_file)
    validator.validate(xml_file)
    
if __name__ == "__main__":
    main()