"""Run each firmware scenario in a fresh process; pass native executable path."""
import subprocess
import sys
SCENARIOS = ['normal', 'stop_approach', 'slow', 'under_arm', 'noisy_approach',
             'exit_first', 'reverse', 'abandon', 'tailgate', 'sonar_disconnect',
             'beam_disconnect', 'beam_short', 'rapid', 'reopen',
             'single_invalid_closing', 'jam', 'limits', 'manual', 'dwell',
             'hysteresis', 'echo', 'echo_timeout', 'rollover', 'random',
             'reset_open', 'reset_mid', 'reset_blocked', 'arrival_timeout',
             'lost_endpoint', 'stale']
for scenario in SCENARIOS:
    subprocess.run([sys.argv[1], scenario], check=True)
print(f'{len(SCENARIOS)}/{len(SCENARIOS)} software scenarios passed. Hardware NOT tested.')
