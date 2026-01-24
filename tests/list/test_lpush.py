import pytest
from tellypy import Client

import sys
from pathlib import Path

utils_path = Path(__file__).resolve().parent.parent
sys.path.append(str(utils_path))

try:
    from utils import wrong_argument, invalid_type, ExtendedTestCase
except ImportError:
    pass


class LPushCommand(ExtendedTestCase):
    client: Client

    @classmethod
    def setUpClass(self):
        self.client = Client(host="localhost", port=6379)
        self.client.connect()

        self.client.execute_command("DEL list")

    def test_unexisted_list(self):
        self.assertIntegerEqual("LPUSH list value", 1)

    @pytest.mark.order(1)
    @pytest.mark.dependency(name="unexisted_multi_values")
    def test_unexisted_multi_values(self):
        self.assertIntegerEqual("LPUSH list str 1 true null", 4)

    @pytest.mark.order(2)
    @pytest.mark.dependency(depends=["unexisted_multi_values"])
    def test_direction(self):
        self.assertIntegerEqual("LPUSH list str 1 true null", 4)
        self.client.execute_command("RPUSH list val")

        self.assertNullEqual("LPOP list")
        self.assertBooleanEqual("LPOP list", True)
        self.assertIntegerEqual("LPOP list", 1)
        self.assertBulkStringEqual("LPOP list", "str")
        self.assertBulkStringEqual("LPOP list", "val")

    def test_existed_list(self):
        self.client.execute_command("LPUSH list value")

        self.assertIntegerEqual("LPUSH list another_value", 1)
        self.assertIntegerEqual("LLEN list", 2)

    def test_existed_multi_values(self):
        self.client.execute_command("LPUSH list value")

        self.assertIntegerEqual("LPUSH list str 1 true null", 4)
        self.assertIntegerEqual("LLEN list", 5)

    def test_wrong_argument_count(self):
        self.assertSimpleErrorEqual("LPUSH", wrong_argument("LPUSH"))
        self.assertSimpleErrorEqual("LPUSH list", wrong_argument("LPUSH"))

    def test_invalid_type(self):
        self.client.execute_command("SET list 12")

        self.assertSimpleErrorEqual("LPUSH list value", invalid_type("LPUSH"))

    def tearDown(self):
        self.client.execute_command("DEL list")
