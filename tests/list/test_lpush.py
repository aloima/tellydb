import pytest
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


class LPushCommand(unittest.TestCase):
    client: Client

    @classmethod
    def setUpClass(self):
        self.client = Client(host="localhost", port=6379)
        self.client.connect()

        self.client.execute_command("DEL list")

    def test_unexisted_list(self):
        response = self.client.execute_command("LPUSH list value")
        self.assertEqual(response.kind, Kind.INTEGER)
        self.assertIsInstance(response.data, int)
        self.assertEqual(response.data, 1)

    @pytest.mark.order(1)
    @pytest.mark.dependency(name="unexisted_multi_values")
    def test_unexisted_multi_values(self):
        response = self.client.execute_command("LPUSH list str 1 true null")
        self.assertEqual(response.kind, Kind.INTEGER)
        self.assertIsInstance(response.data, int)
        self.assertEqual(response.data, 4)

    @pytest.mark.order(2)
    @pytest.mark.dependency(depends=["unexisted_multi_values"])
    def test_direction(self):
        response = self.client.execute_command("LPUSH list str 1 true null")
        self.assertEqual(response.kind, Kind.INTEGER)
        self.assertIsInstance(response.data, int)
        self.assertEqual(response.data, 4)

        self.client.execute_command("RPUSH list val")

        self.assertEqual(self.client.execute_command("LPOP list").data, None)
        self.assertEqual(self.client.execute_command("LPOP list").data, True)
        self.assertEqual(self.client.execute_command("LPOP list").data, 1)
        self.assertEqual(self.client.execute_command("LPOP list").data, "str")
        self.assertEqual(self.client.execute_command("LPOP list").data, "val")

    def test_existed_list(self):
        self.client.execute_command("LPUSH list value")

        response = self.client.execute_command("LPUSH list another_value")
        self.assertEqual(response.kind, Kind.INTEGER)
        self.assertIsInstance(response.data, int)
        self.assertEqual(response.data, 1)

        self.assertEqual(self.client.execute_command("LLEN list").data, 2)

    def test_existed_multi_values(self):
        self.client.execute_command("LPUSH list value")

        response = self.client.execute_command("LPUSH list str 1 true null")
        self.assertEqual(response.kind, Kind.INTEGER)
        self.assertIsInstance(response.data, int)
        self.assertEqual(response.data, 4)

        self.assertEqual(self.client.execute_command("LLEN list").data, 5)

    def test_wrong_argument_count(self):
        response = self.client.execute_command("LPUSH")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, wrong_argument("LPUSH"))

        response = self.client.execute_command("LPUSH list")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, wrong_argument("LPUSH"))

    def test_invalid_type(self):
        self.client.execute_command("SET list 12")

        response = self.client.execute_command("LPUSH list value")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, invalid_type("LPUSH"))

    def tearDown(self):
        self.client.execute_command("DEL list")
