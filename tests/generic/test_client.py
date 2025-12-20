import unittest
import redis
from redis.exceptions import ResponseError

import sys
from pathlib import Path

constants_path = Path(__file__).resolve().parent.parent
sys.path.append(str(constants_path))

try:
    from constants import wrong_argument, out_of_bounds
except ImportError:
    pass


class ClientCommand(unittest.TestCase):
    client: redis.Redis
    other_client: redis.Redis

    @classmethod
    def setUpClass(self):
        self.client = redis.Redis(host="localhost", port=6379, db=0)
        self.other_client = redis.Redis(host="localhost", port=6379, db=0)

    def test_id_argument(self):
        response = self.client.execute_command("CLIENT ID")
        self.assertIsInstance(response, int)
        self.assertGreater(response, 0)

    def test_list_argument(self):
        response = self.client.execute_command("CLIENT LIST")
        self.assertTrue(isinstance(response, list))

        for element in response:
            self.assertIsInstance(element, bytes)
            self.assertTrue(element.isdigit())

    def test_info_argument(self):
        response = self.client.execute_command("CLIENT INFO")
        self.assertIsInstance(response, bytes)

        response = self.client.execute_command(
            "CLIENT INFO", str(self.other_client.client_id())
        )

        self.assertIsInstance(response, bytes)

    def test_info_argument_unexisted(self):
        with self.assertRaises(ResponseError) as e:
            self.client.execute_command("CLIENT INFO", "123456789")

        self.assertEqual(
            e.exception.args[0],
            "The client does not exist"
        )

    def test_info_argument_out_of_bounds(self):
        with self.assertRaises(ResponseError) as e1:
            value = (1 << 32)
            self.client.execute_command("CLIENT INFO", str(value))

        self.assertEqual(
            e1.exception.args[0],
            "Specified ID is out of bounds for uint32_t"
        )

        with self.assertRaises(ResponseError) as e2:
            value = -1
            self.client.execute_command("CLIENT INFO", str(value))

        self.assertEqual(
            e2.exception.args[0],
            "Specified ID is out of bounds for uint32_t"
        )

    def test_info_argument_exceed_arguments(self):
        with self.assertRaises(ResponseError) as e:
            self.client.execute_command("CLIENT INFO", "1", "2")

        self.assertEqual(
            e.exception.args[0],
            "The argument count must be 1 or 2."
        )

# Requires permissions integration
#    def test_kill_argument(self):
#        response = self.client.execute_command(
#            "CLIENT KILL", self.other_client.client_id()
#        )
#
#        self.assertEqual(response, b"OK")

    def test_kill_argument_out_of_bounds(self):
        with self.assertRaises(ResponseError) as e1:
            value = (1 << 32)
            self.client.execute_command("CLIENT KILL", str(value))

        with self.assertRaises(ResponseError) as e2:
            value = -1
            self.client.execute_command("CLIENT KILL", str(value))

        self.assertEqual(e1.exception.args[0], out_of_bounds)
        self.assertEqual(e2.exception.args[0], out_of_bounds)

    def test_kill_argument_invalid(self):
        with self.assertRaises(ResponseError) as e1:
            self.client.execute_command("CLIENT KILL", "abcd")

        self.assertEqual(
            e1.exception.args[0],
            "Specified argument must be integer for the ID"
        )

    def test_kill_argument_unexisted(self):
        with self.assertRaises(ResponseError) as e1:
            self.client.execute_command("CLIENT KILL", "123456789")

        self.assertEqual(
            e1.exception.args[0],
            "There is no client whose ID is #123456789"
        )

    def test_setinfo_argument(self):
        response = self.client.execute_command(
            "CLIENT SETINFO", "LIB-NAME", "telly-cli"
        )

        self.assertIsInstance(response, bytes)
        self.assertEqual(response, b"OK")

        response = self.client.execute_command(
            "CLIENT SETINFO", "LIB-VERSION", "1.0"
        )

        self.assertIsInstance(response, bytes)
        self.assertEqual(response, b"OK")

        response = self.client.execute_command("CLIENT INFO")
        self.assertRegex(str(response), r"Library name: telly-cli")
        self.assertRegex(str(response), r"Library version: 1.0")

    def test_setinfo_argument_invalid(self):
        with self.assertRaises(ResponseError) as e:
            self.client.execute_command("CLIENT SETINFO", "INVALID", "value")

        self.assertEqual(e.exception.args[0], "Unknown property")

    def test_setinfo_argument_wrong_argument_count(self):
        with self.assertRaises(ResponseError) as e:
            self.client.execute_command("CLIENT SETINFO", "LIB-NAME")

        self.assertEqual(e.exception.args[0], wrong_argument("CLIENT SETINFO"))
