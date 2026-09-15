#!/usr/bin/env python3
"""Real runtime lifecycle consistency when the record allocator is exhausted."""
from test_telemetry_runtime_integration import main

if __name__ == "__main__":
    main(exhaustion=True)
