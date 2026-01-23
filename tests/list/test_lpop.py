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


class LPopCommand(unittest.TestCase):
    client: Client

    @classmethod
    def setUpClass(self):
        self.client = Client(host="localhost", port=6379)
        self.client.connect()

        self.client.execute_command("DEL list")

    def test_wrong_arguments(self):
        response = self.client.execute_command("LPOP")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, wrong_argument("LPOP"))

        response = self.client.execute_command("LPOP list a b")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, wrong_argument("LPOP"))

    def test_unexisted_list(self):
        response = self.client.execute_command("LPOP list")
        self.assertEqual(response.kind, Kind.NULL)
        self.assertIsInstance(response.data, type(None))
        self.assertEqual(response.data, None)

    def test_not_list(self):
        self.client.execute_command("SET list value")

        response = self.client.execute_command("LPOP list")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, invalid_type("LPOP"))

    def test_ordered_list(self):
        # value_a value_b value_c value_d value_e
        self.client.execute_command("RPUSH list value_a")
        self.client.execute_command("RPUSH list value_b")
        self.client.execute_command("RPUSH list value_c")
        self.client.execute_command("RPUSH list value_d")
        self.client.execute_command("RPUSH list value_e")

        for value in ["value_a", "value_b", "value_c", "value_d", "value_e"]:
            response = self.client.execute_command("LPOP list")
            self.assertEqual(response.kind, Kind.BULK_STRING)
            self.assertIsInstance(response.data, str)
            self.assertEqual(response.data, value)

        response = self.client.execute_command("LPOP list")
        self.assertEqual(response.kind, Kind.NULL)
        self.assertIsInstance(response.data, type(None))
        self.assertEqual(response.data, None)

    def test_reverse_ordered_list(self):
        # value_e value_d value_c value_b value_a
        self.client.execute_command("LPUSH list value_a")
        self.client.execute_command("LPUSH list value_b")
        self.client.execute_command("LPUSH list value_c")
        self.client.execute_command("LPUSH list value_d")
        self.client.execute_command("LPUSH list value_e")

        for value in ["value_e", "value_d", "value_c", "value_b", "value_a"]:
            response = self.client.execute_command("LPOP list")
            self.assertEqual(response.kind, Kind.BULK_STRING)
            self.assertIsInstance(response.data, str)
            self.assertEqual(response.data, value)

        response = self.client.execute_command("LPOP list")
        self.assertEqual(response.kind, Kind.NULL)
        self.assertIsInstance(response.data, type(None))
        self.assertEqual(response.data, None)

    def test_unordered_list(self):
        # value_e value_b value_a value_c value_d
        self.client.execute_command("LPUSH list value_a")
        self.client.execute_command("LPUSH list value_b")
        self.client.execute_command("RPUSH list value_c")
        self.client.execute_command("RPUSH list value_d")
        self.client.execute_command("LPUSH list value_e")

        for value in ["value_e", "value_b", "value_a", "value_c", "value_d"]:
            response = self.client.execute_command("LPOP list")
            self.assertEqual(response.kind, Kind.BULK_STRING)
            self.assertIsInstance(response.data, str)
            self.assertEqual(response.data, value)

        response = self.client.execute_command("LPOP list")
        self.assertEqual(response.kind, Kind.NULL)
        self.assertIsInstance(response.data, type(None))
        self.assertEqual(response.data, None)

    def tearDown(self):
        self.client.execute_command("DEL list")
