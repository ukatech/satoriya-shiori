#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Unit test suite for Satori Validator (`tests/test_validator.py`).
"""

import unittest
from pathlib import Path
from tests.test_validator import (
    detect_encoding,
    validate_satori_file,
    LEVEL_ERROR,
    LEVEL_WARN
)

class TestSatoriValidator(unittest.TestCase):
    def setUp(self):
        self.fixtures_dir = Path(__file__).parent / 'fixtures'

    def test_valid_utf8(self):
        filepath = self.fixtures_dir / 'valid_utf8.txt'
        res = validate_satori_file(filepath)
        self.assertEqual(res.encoding, 'UTF-8')
        self.assertEqual(res.error_count, 0)
        self.assertEqual(res.status, 'PASS')

    def test_valid_utf8_bom(self):
        filepath = self.fixtures_dir / 'valid_utf8_bom.txt'
        res = validate_satori_file(filepath)
        self.assertEqual(res.encoding, 'UTF-8 BOM')
        self.assertEqual(res.error_count, 0)
        self.assertEqual(res.status, 'PASS')

    def test_valid_cp932(self):
        filepath = self.fixtures_dir / 'valid_cp932.txt'
        res = validate_satori_file(filepath)
        self.assertEqual(res.encoding, 'CP932')
        self.assertEqual(res.error_count, 0)
        self.assertEqual(res.status, 'PASS')

    def test_invalid_unclosed_kakko(self):
        filepath = self.fixtures_dir / 'invalid_unclosed_kakko.txt'
        res = validate_satori_file(filepath)
        self.assertEqual(res.error_count, 1)
        self.assertEqual(res.status, 'FAIL')
        self.assertEqual(res.issues[0].level, LEVEL_ERROR)

    def test_valid_escaped_kakko(self):
        filepath = self.fixtures_dir / 'valid_escaped_kakko.txt'
        res = validate_satori_file(filepath)
        self.assertEqual(res.error_count, 0)
        self.assertEqual(res.status, 'PASS')

    def test_valid_halfwidth_brackets(self):
        filepath = self.fixtures_dir / 'valid_halfwidth_brackets.txt'
        res = validate_satori_file(filepath)
        self.assertEqual(res.error_count, 0)
        self.assertEqual(res.status, 'PASS')

    def test_empty_file(self):
        filepath = self.fixtures_dir / 'empty_file.txt'
        res = validate_satori_file(filepath)
        self.assertEqual(res.encoding, 'Empty')
        self.assertEqual(res.error_count, 0)
        self.assertEqual(res.warn_count, 1)
        self.assertEqual(res.status, 'WARN')

if __name__ == '__main__':
    unittest.main()
