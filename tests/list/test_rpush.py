import sys
from pathlib import Path

import pytest
from tellypy import Client

utils_path = Path(__file__).resolve().parent.parent
sys.path.append(str(utils_path))

try:
    from utils import ExtendedTestCase, invalid_type, wrong_argument
except ImportError:
    sys.exit(1)


class RPushCommand(ExtendedTestCase):
    client: Client

    @classmethod
    def setUpClass(cls):
        cls.client = Client(host="localhost", port=6379)
        cls.client.connect()

        cls.client.execute_command("DEL list")

    def test_unexisted_list(self):
        self.assertIntegerEqual("RPUSH list value", 1)

    @pytest.mark.order(1)
    @pytest.mark.dependency(name="unexisted_multi_values")
    def test_unexisted_multi_values(self):
        self.assertIntegerEqual("RPUSH list str 1 true null", 4)

    @pytest.mark.order(2)
    @pytest.mark.dependency(depends=["unexisted_multi_values"])
    def test_direction(self):
        self.assertIntegerEqual("RPUSH list str 1 true null", 4)
        self.client.execute_command("LPUSH list val")

        self.assertNullEqual("RPOP list")
        self.assertBooleanEqual("RPOP list", True)
        self.assertIntegerEqual("RPOP list", 1)
        self.assertBulkStringEqual("RPOP list", "str")
        self.assertBulkStringEqual("RPOP list", "val")

    def test_existed_list(self):
        self.client.execute_command("RPUSH list value")

        self.assertIntegerEqual("RPUSH list another_value", 1)
        self.assertIntegerEqual("LLEN list", 2)

    def test_existed_multi_values(self):
        self.client.execute_command("RPUSH list value")

        self.assertIntegerEqual("RPUSH list str 1 true null", 4)
        self.assertIntegerEqual("LLEN list", 5)

    def test_wrong_argument_count(self):
        self.assertSimpleErrorEqual("RPUSH", wrong_argument("RPUSH"))
        self.assertSimpleErrorEqual("RPUSH list", wrong_argument("RPUSH"))

    def test_invalid_type(self):
        self.client.execute_command("SET list 12")
        self.assertSimpleErrorEqual("RPUSH list value", invalid_type("RPUSH"))

    def tearDown(self):
        self.client.execute_command("DEL list")
