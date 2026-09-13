"""Run each firmware scenario in a fresh process; pass native executable path."""
import subprocess
import sys
SCENARIOS = ['normal', 'stop_approach', 'slow', 'under_arm', 'noisy_approach',
             'exit_first', 'reverse', 'abandon', 'tailgate', 'sonar_disconnect',
             'beam_disconnect', 'beam_short', 'rapid', 'reopen',
             'single_invalid_closing', 'jam', 'limits', 'manual', 'dwell',
             'hysteresis', 'echo', 'echo_timeout', 'rollover', 'random',
             'reset_open', 'reset_mid', 'reset_blocked', 'arrival_timeout',
             'lost_endpoint', 'stale', 'long_stop_recovers', 'beam_monitor_closing',
             'reversal_endpoint', 'reverse_source_coast', 'late_echo',
             'buffered_echo_stale', 'preclose_fresh_interval', 'single_near_closing',
             'closing_speed', 'sonar_fault_closing', 'raw_endpoint_veto',
             'obstruction_withdrawn_reopen', 'beam_false_clear_pulse',
             'closed_endpoint_missing', 'repeated_cycles', 'timing_boundaries']
for scenario in SCENARIOS:
    subprocess.run([sys.argv[1], scenario], check=True, timeout=10)
print(f'{len(SCENARIOS)}/{len(SCENARIOS)} software scenarios passed. Hardware NOT tested.')
