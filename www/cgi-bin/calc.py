#!/usr/bin/python3
import os, sys
from urllib.parse import parse_qs

q = parse_qs(os.environ.get("QUERY_STRING", ""))
def num(name):
    try:
        return float(q.get(name, ["0"])[0])
    except ValueError:
        return 0.0
a, b = num("a"), num("b")
op = q.get("op", ["add"])[0]
ops = {"add": a + b, "sub": a - b, "mul": a * b,
       "div": (a / b if b != 0 else float("nan"))}
res = ops.get(op, a + b)
if res == int(res):
    res = int(res)

sys.stdout.write("Content-Type: application/json\r\n\r\n")
sys.stdout.write('{"a": %s, "b": %s, "op": "%s", "result": %s}\n' % (a, b, op, res))
