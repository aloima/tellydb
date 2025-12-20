import unittest
import redis
from redis.exceptions import ResponseError
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
    @classmethod
    def setUpClass(self):
        self.client = redis.Redis(host="localhost", port=6379, db=0)

    # If command respond b"PONG", returns True, otherwise returns False
    def test_without_arguments(self):
        response = self.client.execute_command("PING")
        self.assertTrue(response)

    # If command respond b"PONG", returns True, otherwise returns False
    def test_with_argument(self):
        value = "".join(random.choices(ascii_letters + digits, k=64))
        response = self.client.execute_command("PING", value)
        self.assertFalse(response)

    def test_with_arguments(self):
        with self.assertRaises(ResponseError) as e:
            self.client.execute_command("PING", "a", "b")

        self.assertEqual(e.exception.args[0], wrong_argument("PING"))
