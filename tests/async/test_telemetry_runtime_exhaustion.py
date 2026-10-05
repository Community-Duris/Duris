#!/usr/bin/env python3
"""Real runtime lifecycle consistency when the record allocator is exhausted."""
from test_telemetry_runtime_integration import main
import argparse
from pathlib import Path

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--clock-performance-output", type=Path)
    args = parser.parse_args()
    main(exhaustion=True, sanitize=args.sanitize, clock_output=args.clock_performance_output)
