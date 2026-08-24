#!/bin/bash
# clear all redis cache for durismud

echo "clearing redis cache..."
redis-cli FLUSHDB
echo "done."
