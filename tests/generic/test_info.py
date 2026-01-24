import pytest
from tellypy import Client, Kind

import sys
from pathlib import Path

utils_path = Path(__file__).resolve().parent.parent
sys.path.append(str(utils_path))

try:
    from utils import ExtendedTestCase
except ImportError:
    pass


class InfoCommand(ExtendedTestCase):
    client: Client
    sections = ["server", "clients"]

    @classmethod
    def setUpClass(self):
        self.client = Client(host="localhost", port=6379)
        self.client.connect()

    @pytest.mark.order(1)
    @pytest.mark.dependency(name="no_section")
    def test_no_section(self):
        response = self.client.execute_command("INFO")
        self.assertEqual(response.kind, Kind.BULK_STRING)
        self.assertIsInstance(response.data, str)
        self.assertGreater(len(response.data), 0)

    def test_one_section(self):
        for section in self.sections:
            response = self.client.execute_command(f"INFO {section}")
            self.assertEqual(response.kind, Kind.BULK_STRING)
            self.assertIsInstance(response.data, str)
            self.assertGreater(len(response.data), 0)

    @pytest.mark.order(2)
    @pytest.mark.dependency(depends=["no_section"])
    def test_all_sections(self):
        response_all = self.client.execute_command(f"INFO {' '.join(self.sections)}")

        self.assertEqual(response_all.kind, Kind.BULK_STRING)
        self.assertIsInstance(response_all.data, str)
        self.assertGreater(len(response_all.data), 0)

        # Total processed transactions field must be processed
        # response = self.client.execute_command("INFO")
        # self.assertEqual(response_all.data, response.data)

    def test_invalid_section(self):
        self.assertSimpleErrorEqual("INFO unknown", "Invalid section name")
