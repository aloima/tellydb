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


class LLenCommand(unittest.TestCase):
    client: Client

    @classmethod
    def setUpClass(self):
        self.client = Client(host="localhost", port=6379)
        self.client.connect()

        self.client.execute_command("DEL list")

    def test_existed_list(self):
        self.client.execute_command("RPUSH list value_a")
        self.client.execute_command("LPUSH list value_b")

        response = self.client.execute_command("LLEN list")
        self.assertEqual(response.kind, Kind.INTEGER)
        self.assertIsInstance(response.data, int)
        self.assertEqual(response.data, 2)

    def test_unexisted_list(self):
        response = self.client.execute_command("LLEN list")
        self.assertEqual(response.kind, Kind.INTEGER)
        self.assertIsInstance(response.data, int)
        self.assertEqual(response.data, 0)

    def test_not_list(self):
        self.client.execute_command("SET list value")

        response = self.client.execute_command("LLEN list")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, invalid_type("LLEN"))

    def test_wrong_arguments(self):
        response = self.client.execute_command("LLEN")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, wrong_argument("LLEN"))

        response = self.client.execute_command("LLEN list_a list_b")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, wrong_argument("LLEN"))

    def tearDown(self):
        self.client.execute_command("DEL list")
