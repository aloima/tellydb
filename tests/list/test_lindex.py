from tellypy import Client, Kind

import sys
from pathlib import Path

utils_path = Path(__file__).resolve().parent.parent
sys.path.append(str(utils_path))

try:
    from utils import wrong_argument, invalid_type, ExtendedTestCase
except ImportError:
    pass


class LIndexCommand(ExtendedTestCase):
    client: Client

    @classmethod
    def setUpClass(self):
        self.client = Client(host="localhost", port=6379)
        self.client.connect()

        self.client.execute_command("DEL list")

    def test_positive_index(self):
        self.client.execute_command("RPUSH list value_a value_b value_c value_d value_e")

        self.assertBulkStringEqual("LINDEX list 0", "value_a")
        self.assertBulkStringEqual("LINDEX list 3", "value_d")

    def test_negative_index(self):
        self.client.execute_command("RPUSH list value_a value_b value_c value_d value_e")

        self.assertBulkStringEqual("LINDEX list -1", "value_e")
        self.assertBulkStringEqual("LINDEX list -3", "value_c")

    def test_invalid_index(self):
        self.client.execute_command("RPUSH list value_a value_b value_c value_d value_e")

        self.assertNullEqual("LINDEX list 20")
        self.assertNullEqual("LINDEX list -20")

    def test_wrong_argument_count(self):
        self.client.execute_command("RPUSH list value_a")

        self.assertSimpleErrorEqual("LINDEX list", wrong_argument("LINDEX"))
        self.assertSimpleErrorEqual("LINDEX list 0 0", wrong_argument("LINDEX"))

    def test_integer_bounds(self):
        self.client.execute_command("RPUSH list value_a")

        # (1 << 64) = ULLONG_MAX + 1
        self.assertSimpleErrorEqual(f"LINDEX list {1 << 64}", "Index exceeded integer bounds")
        self.assertSimpleErrorEqual(f"LINDEX list -{1 << 64}", "Index exceeded integer bounds")

        response = self.client.execute_command(f"LINDEX list {(1 << 64) - 2}")
        self.assertNotEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertNotIsInstance(response.data, str)
        self.assertNotEqual(response.data, "Index exceeded integer bounds")

        response = self.client.execute_command(f"LINDEX list -{(1 << 64) - 2}")
        self.assertNotEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertNotIsInstance(response.data, str)
        self.assertNotEqual(response.data, "Index exceeded integer bounds")

    def test_not_integer(self):
        self.client.execute_command("RPUSH list value_a")

        self.assertSimpleErrorEqual("LINDEX list a", "Second argument must be an integer")

    def test_unexisted_list(self):
        self.assertSimpleErrorEqual("LINDEX list 0", invalid_type("LINDEX"))

    def test_not_list(self):
        self.client.execute_command("SET list value")

        self.assertSimpleErrorEqual("LINDEX list 0", invalid_type("LINDEX"))

    def tearDown(self):
        self.client.execute_command("DEL list")
