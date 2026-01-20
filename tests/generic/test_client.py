import unittest
import pytest
from tellypy import Client, Kind

import sys
from pathlib import Path

constants_path = Path(__file__).resolve().parent.parent
sys.path.append(str(constants_path))

try:
    from constants import wrong_argument, OUT_OF_BOUNDS
except ImportError:
    pass


class ClientCommand(unittest.TestCase):
    client: Client
    other_client: Client

    @classmethod
    def setUpClass(self):
        self.client = Client(host="localhost", port=6379)
        self.other_client = Client(host="localhost", port=6379)

        self.client.connect()
        self.other_client.connect()

    @pytest.mark.dependency(name="id")
    def test_id_argument(self):
        response = self.client.execute_command("CLIENT ID")
        self.assertEqual(response.kind, Kind.INTEGER)
        self.assertIsInstance(response.data, int)
        self.assertGreater(response.data, 0)

    def test_list_argument(self):
        response = self.client.execute_command("CLIENT LIST")
        self.assertEqual(response.kind, Kind.ARRAY)
        self.assertIsInstance(response.data, list)

        for element in response.data:
            self.assertEqual(element.kind, Kind.SIMPLE_STRING)
            self.assertIsInstance(element.data, str)

    @pytest.mark.dependency(depends=["id"])
    def test_info_argument(self):
        response = self.client.execute_command("CLIENT INFO")
        self.assertEqual(response.kind, Kind.BULK_STRING)
        self.assertIsInstance(response.data, str)

        response = self.client.execute_command(
            f"CLIENT INFO {self.other_client.get_id()}"
        )

        self.assertEqual(response.kind, Kind.BULK_STRING)
        self.assertIsInstance(response.data, str)

    def test_info_argument_unexisted(self):
        response = self.client.execute_command("CLIENT INFO 123456789")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)

        self.assertEqual(response.data, "The client does not exist")

    def test_info_argument_out_of_bounds(self):
        response = self.client.execute_command(f"CLIENT INFO {1 << 32}")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)

        self.assertEqual(response.data, OUT_OF_BOUNDS)

        response = self.client.execute_command("CLIENT INFO -1")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)

        self.assertEqual(response.data, OUT_OF_BOUNDS)

    def test_info_argument_exceed_arguments(self):
        response = self.client.execute_command("CLIENT INFO 1 2")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)

        self.assertEqual(response.data, "The argument count must be 1 or 2.")

# Requires permissions integration
#    @pytest.mark.dependency(depends=["id"])
#    def test_kill_argument(self):
#        response = self.client.execute_command(
#            "CLIENT KILL", self.other_client.get_id()
#        )
#
#        self.assertEqual(response, b"OK")

    def test_kill_argument_out_of_bounds(self):
        response = self.client.execute_command(f"CLIENT KILL {1 << 32}")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)

        self.assertEqual(response.data, OUT_OF_BOUNDS)

        response = self.client.execute_command("CLIENT KILL -1")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)

        self.assertEqual(response.data, OUT_OF_BOUNDS)

    def test_kill_argument_invalid(self):
        response = self.client.execute_command("CLIENT KILL abcd")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)

        self.assertEqual(
            response.data,
            "Specified argument must be integer for the ID"
        )

    def test_kill_argument_unexisted(self):
        response = self.client.execute_command("CLIENT KILL 123456789")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)

        self.assertEqual(
            response.data,
            "There is no client whose ID is #123456789"
        )

    def test_setinfo_argument(self):
        response = self.client.execute_command(
            "CLIENT SETINFO LIB-NAME telly-cli"
        )

        self.assertEqual(response.kind, Kind.SIMPLE_STRING)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, "OK")

        response = self.client.execute_command(
            "CLIENT SETINFO LIB-VERSION 1.0"
        )

        self.assertEqual(response.kind, Kind.SIMPLE_STRING)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, "OK")

        response = self.client.execute_command("CLIENT INFO")
        self.assertRegex(response.data, r"Library name: telly-cli")
        self.assertRegex(response.data, r"Library version: 1.0")

    def test_setinfo_argument_invalid(self):
        response = self.client.execute_command("CLIENT SETINFO INVALID value")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)

        self.assertEqual(response.data, "Unknown property")

    def test_setinfo_argument_wrong_argument_count(self):
        response = self.client.execute_command("CLIENT SETINFO LIB-NAME")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)

        self.assertEqual(response.data, wrong_argument("CLIENT SETINFO"))
