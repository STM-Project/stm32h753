#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
export PYTHONDONTWRITEBYTECODE=1
cloud_test_dir=$(mktemp -d /tmp/h7-google-tests.XXXXXX)
trap 'rm -rf "$cloud_test_dir"' EXIT
for cloud_test in google_cloud_protocol google_cloud_at; do
  gcc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -IApplication/GOOGLE \
    "tests/${cloud_test}_test.c" Application/GOOGLE/google_cloud_protocol.c Application/GOOGLE/google_cloud_at.c \
    -o "$cloud_test_dir/$cloud_test"
  "$cloud_test_dir/$cloud_test"
done
python3 tests/test_google_cloud_fsm.py
node tests/google_apps_script_test.js
python3 - <<'PY'
import ssl
context=ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
context.load_verify_locations(cafile='cloud/google-apps-script/google_gts_roots.pem')
names = {value for cert in context.get_ca_certs() for rdn in cert['subject']
         for field, value in rdn if field == 'commonName'}
assert {f'GTS Root R{number}' for number in range(1, 5)} <= names
print('PASS: GTS CA bundle loads all four trust anchors.')
PY
