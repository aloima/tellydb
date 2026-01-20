import unittest
from tellypy import Client, Kind
import random
from string import ascii_letters, digits

import sys
from pathlib import Path

constants_path = Path(__file__).resolve().parent.parent
sys.path.append(str(constants_path))

try:
    from constants import wrong_argument
except ImportError:
    pass


class PingCommand(unittest.TestCase):
    client: Client

    @classmethod
    def setUpClass(self):
        self.client = Client(host="localhost", port=6379)
        self.client.connect()

    def test_without_arguments(self):
        response = self.client.execute_command("PING")
        self.assertEqual(response.kind, Kind.SIMPLE_STRING)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, "PONG")

    def test_with_argument(self):
        value = "".join(random.choices(ascii_letters + digits, k=64))
        response = self.client.execute_command(f"PING {value}")
        self.assertEqual(response.kind, Kind.BULK_STRING)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, value)

    def test_with_arguments(self):
        response = self.client.execute_command("PING a b")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, wrong_argument("PING"))
