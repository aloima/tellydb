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


class StatusCommand(ExtendedTestCase):
    client: Client

    @classmethod
    def setUpClass(self):
        self.client = Client(host="localhost", port=6379)
        self.client.connect()

    def test_status(self):
        response = self.client.execute_command("STATUS")
        self.assertEqual(response.kind, Kind.BULK_STRING)
        self.assertIsInstance(response.data, str)
        self.assertIn("Status: online", response.data)
        self.assertIn("LastErrorDate", response.data)
