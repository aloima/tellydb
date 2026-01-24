import pytest
from tellypy import Client, Kind

import sys
from pathlib import Path

utils_path = Path(__file__).resolve().parent.parent
sys.path.append(str(utils_path))

try:
    from utils import wrong_argument, OUT_OF_BOUNDS, ExtendedTestCase
except ImportError:
    pass


class ClientCommand(ExtendedTestCase):
    client: Client
    other_client: Client

    @classmethod
    def setUpClass(self):
        self.client = Client(host="localhost", port=6379)
        self.other_client = Client(host="localhost", port=6379)

        self.client.connect()
        self.other_client.connect()

    @pytest.mark.order(1)
    @pytest.mark.dependency(name="id")
    def test_id_argument(self):
        response = self.client.execute_command("CLIENT ID")
        self.assertEqual(response.kind, Kind.INTEGER)
        self.assertIsInstance(response.data, int)
        self.assertGreaterEqual(response.data, 0)

    def test_list_argument(self):
        response = self.client.execute_command("CLIENT LIST")
        self.assertEqual(response.kind, Kind.ARRAY)
        self.assertIsInstance(response.data, list)

        for element in response.data:
            self.assertEqual(element.kind, Kind.SIMPLE_STRING)
            self.assertIsInstance(element.data, str)

    @pytest.mark.order(2)
    @pytest.mark.dependency(depends=["id"])
    def test_info_argument(self):
        response = self.client.execute_command("CLIENT INFO")
        self.assertEqual(response.kind, Kind.BULK_STRING)
        self.assertIsInstance(response.data, str)

        response = self.client.execute_command(f"CLIENT INFO {self.other_client.get_id()}")
        self.assertEqual(response.kind, Kind.BULK_STRING)
        self.assertIsInstance(response.data, str)

    def test_info_argument_unexisted(self):
        self.assertSimpleErrorEqual("CLIENT INFO 123456789", "The client does not exist")

    def test_info_argument_out_of_bounds(self):
        self.assertSimpleErrorEqual(f"CLIENT INFO {1 << 32}", OUT_OF_BOUNDS)
        self.assertSimpleErrorEqual("CLIENT INFO -1", OUT_OF_BOUNDS)

    def test_info_argument_exceed_arguments(self):
        self.assertSimpleErrorEqual("CLIENT INFO 1 2", "The argument count must be 1 or 2.")

# Requires permissions integration
#    @pytest.mark.order(2)
#    @pytest.mark.dependency(depends=["id"])
#    def test_kill_argument(self):
#        response = self.client.execute_command(
#            "CLIENT KILL", self.other_client.get_id()
#        )
#
#        self.assertEqual(response, b"OK")

    def test_kill_argument_out_of_bounds(self):
        self.assertSimpleErrorEqual(f"CLIENT KILL {1 << 32}", OUT_OF_BOUNDS)
        self.assertSimpleErrorEqual("CLIENT KILL -1", OUT_OF_BOUNDS)

    def test_kill_argument_invalid(self):
        self.assertSimpleErrorEqual("CLIENT KILL abcd", "Specified argument must be integer for the ID")

    def test_kill_argument_unexisted(self):
        c_id = "123456789"
        self.assertSimpleErrorEqual(f"CLIENT KILL {c_id}", f"There is no client whose ID is #{c_id}")

    def test_setinfo_argument(self):
        self.assertSimpleStringEqual("CLIENT SETINFO LIB-NAME telly-cli", "OK")
        self.assertSimpleStringEqual("CLIENT SETINFO LIB-VERSION 1.0", "OK")

        response = self.client.execute_command("CLIENT INFO")
        self.assertRegex(response.data, r"Library name: telly-cli")
        self.assertRegex(response.data, r"Library version: 1.0")

    def test_setinfo_argument_invalid(self):
        self.assertSimpleErrorEqual("CLIENT SETINFO INVALID value", "Unknown property")

    def test_setinfo_argument_wrong_argument_count(self):
        self.assertSimpleErrorEqual("CLIENT SETINFO LIB-NAME", wrong_argument("CLIENT SETINFO"))
