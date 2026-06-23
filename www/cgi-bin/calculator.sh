#!/bin/bash

# CGI header
echo -e "Content-Type: application/json\r"
echo -e "\r"

# get QUERY_STRING
QUERY_STRING="$QUERY_STRING"

# simple parser
get_param() {
    echo "$QUERY_STRING" | tr '&' '\n' | grep "^$1=" | cut -d '=' -f2
}

a=$(get_param a)
b=$(get_param b)
op=$(get_param op)

# default values
a=${a:-0}
b=${b:-0}
op=${op:-add}

# floating safe handling using awk
result=$(awk -v a="$a" -v b="$b" -v op="$op" '
BEGIN {
    if (op == "add") print a + b;
    else if (op == "sub") print a - b;
    else if (op == "mul") print a * b;
    else if (op == "div") {
        if (b == 0) print "nan";
        else print a / b;
    } else print a + b;
}')

# output JSON
echo "{\"a\": $a, \"b\": $b, \"op\": \"$op\", \"result\": $result}"