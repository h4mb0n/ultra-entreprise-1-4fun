#!/usr/bin/env bash
# Usage: ./connect.sh [ESP_IP]
# Default IP can be overridden by UE1_IP env var or first argument.

IP="${1:-${UE1_IP:-192.168.3.68}}"
PORT=23

echo "Connecting to UE1 console at ${IP}:${PORT}"
echo "  BREAK sequence: ~B   (tilde then B)"
echo "  Literal tilde:  ~~"
echo ""

exec socat "TCP:${IP}:${PORT},nodelay" "-,raw,echo=0"
