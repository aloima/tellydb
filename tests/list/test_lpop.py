import sys
from pathlib import Path

from tellypy import Client

utils_path = Path(__file__).resolve().parent.parent
sys.path.append(str(utils_path))

try:
    from utils import ExtendedTestCase, invalid_type, wrong_argument
except ImportError:
    sys.exit(1)


class LPopCommand(ExtendedTestCase):
    client: Client

    @classmethod
    def setUpClass(cls):
        cls.client = Client(host="localhost", port=6379)
        cls.client.connect()

        cls.client.execute_command("DEL list")

    def test_wrong_arguments(self):
        self.assertSimpleErrorEqual("LPOP", wrong_argument("LPOP"))
        self.assertSimpleErrorEqual("LPOP list a b", wrong_argument("LPOP"))

    def test_unexisted_list(self):
        self.assertNullEqual("LPOP list")

    def test_not_list(self):
        self.client.execute_command("SET list value")

        self.assertSimpleErrorEqual("LPOP list", invalid_type("LPOP"))

    def test_ordered_list(self):
        # value_a value_b value_c value_d value_e
        self.client.execute_command(
            "RPUSH list value_a value_b value_c value_d value_e"
        )

        for value in ["value_a", "value_b", "value_c", "value_d", "value_e"]:
            self.assertBulkStringEqual("LPOP list", value)

        self.assertNullEqual("LPOP list")

    def test_reverse_ordered_list(self):
        # value_e value_d value_c value_b value_a
        self.client.execute_command(
            "LPUSH list value_a value_b value_c value_d value_e"
        )

        for value in ["value_e", "value_d", "value_c", "value_b", "value_a"]:
            self.assertBulkStringEqual("LPOP list", value)

        self.assertNullEqual("LPOP list")

    def test_unordered_list(self):
        # value_e value_b value_a value_c value_d
        self.client.execute_command("LPUSH list value_a value_b")
        self.client.execute_command("RPUSH list value_c value_d")
        self.client.execute_command("LPUSH list value_e")

        for value in ["value_e", "value_b", "value_a", "value_c", "value_d"]:
            self.assertBulkStringEqual("LPOP list", value)

        self.assertNullEqual("LPOP list")

    def tearDown(self):
        self.client.execute_command("DEL list")
