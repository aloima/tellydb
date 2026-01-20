import unittest
import pytest
from tellypy import Client, Kind


class InfoCommand(unittest.TestCase):
    client: Client
    sections = ["server", "clients"]

    @classmethod
    def setUpClass(self):
        self.client = Client(host="localhost", port=6379)
        self.client.connect()

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

    @pytest.mark.dependency(depends=["no_section"])
    def test_all_sections(self):
        response_all = self.client.execute_command(
            f"INFO {' '.join(self.sections)}"
        )

        self.assertEqual(response_all.kind, Kind.BULK_STRING)
        self.assertIsInstance(response_all.data, str)
        self.assertGreater(len(response_all.data), 0)

        # Total processed transactions field must be processed
        # response = self.client.execute_command("INFO")
        # self.assertEqual(response_all.data, response.data)

    def test_invalid_section(self):
        response = self.client.execute_command("INFO unknown")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, "Invalid section name")
