import unittest
from tellypy import Client, Kind

import sys
from pathlib import Path

constants_path = Path(__file__).resolve().parent.parent
sys.path.append(str(constants_path))

try:
    from constants import wrong_argument, invalid_type
except ImportError:
    pass


class LIndexCommand(unittest.TestCase):
    client: Client

    @classmethod
    def setUpClass(self):
        self.client = Client(host="localhost", port=6379)
        self.client.connect()

        self.client.execute_command("DEL list")

    def test_positive_index(self):
        self.client.execute_command("RPUSH list value_a")
        self.client.execute_command("RPUSH list value_b")
        self.client.execute_command("RPUSH list value_c")
        self.client.execute_command("RPUSH list value_d")
        self.client.execute_command("RPUSH list value_e")

        response = self.client.execute_command("LINDEX list 0")
        self.assertEqual(response.kind, Kind.BULK_STRING)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, "value_a")

        response = self.client.execute_command("LINDEX list 3")
        self.assertEqual(response.kind, Kind.BULK_STRING)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, "value_d")

    def test_negative_index(self):
        self.client.execute_command("RPUSH list value_a")
        self.client.execute_command("RPUSH list value_b")
        self.client.execute_command("RPUSH list value_c")
        self.client.execute_command("RPUSH list value_d")
        self.client.execute_command("RPUSH list value_e")

        response = self.client.execute_command("LINDEX list -1")
        self.assertEqual(response.kind, Kind.BULK_STRING)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, "value_e")

        response = self.client.execute_command("LINDEX list -3")
        self.assertEqual(response.kind, Kind.BULK_STRING)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, "value_c")

    def test_invalid_index(self):
        self.client.execute_command("RPUSH list value_a")
        self.client.execute_command("RPUSH list value_b")
        self.client.execute_command("RPUSH list value_c")
        self.client.execute_command("RPUSH list value_d")
        self.client.execute_command("RPUSH list value_d")
        self.client.execute_command("RPUSH list value_e")

        response = self.client.execute_command("LINDEX list 20")
        self.assertEqual(response.kind, Kind.NULL)
        self.assertIsInstance(response.data, type(None))
        self.assertEqual(response.data, None)

        response = self.client.execute_command("LINDEX list -20")
        self.assertEqual(response.kind, Kind.NULL)
        self.assertIsInstance(response.data, type(None))
        self.assertEqual(response.data, None)

    def test_wrong_arguments(self):
        self.client.execute_command("RPUSH list value_a")

        response = self.client.execute_command("LINDEX list")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, wrong_argument("LINDEX"))

        response = self.client.execute_command("LINDEX list 0 0")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, wrong_argument("LINDEX"))

    def test_integer_bounds(self):
        self.client.execute_command("RPUSH list value_a")

        # (1 << 64) = ULLONG_MAX + 1
        response = self.client.execute_command(f"LINDEX list {1 << 64}")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, "Index exceeded integer bounds")

        response = self.client.execute_command(f"LINDEX list -{1 << 64}")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, "Index exceeded integer bounds")

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

        response = self.client.execute_command("LINDEX list a")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, "Second argument must be an integer")

    def test_unexisted_list(self):
        response = self.client.execute_command("LINDEX list 0")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, invalid_type("LINDEX"))

    def test_not_list(self):
        self.client.execute_command("SET list value")

        response = self.client.execute_command("LINDEX list 0")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, invalid_type("LINDEX"))

    def tearDown(self):
        self.client.execute_command("DEL list")
