1.2.2 exec tests - run from test-cases directory so P_short, P_prog1, etc. are in cwd.

  cd test-cases
  ../mysh < T_exec_<name>.txt
  # compare with T_exec_<name>_result.txt

  ./run_exec_tests.sh   # runs all and diffs (chmod +x run_exec_tests.sh first)

Tests:
  T_exec_single         exec P_short FCFS (single = same as source)
  T_exec_two            exec P_prog1 P_prog2 FCFS (two programs, FCFS order)
  T_FCFS                exec P_prog1 P_prog2 P_prog3 FCFS (three programs)
  T_exec_invalid_policy exec P_short BADPOLICY then exec P_short FCFS
  T_exec_usage_few      exec P_short (too few args -> Unknown Command)
  T_exec_usage_many     exec with 4 programs (too many -> Unknown Command)
  T_exec_duplicate      exec P_short P_short FCFS (duplicate names error)
  T_exec_notfound       exec NoSuchFile FCFS (file not found) then exec P_short FCFS
  T_exec_policies       exec P_short with FCFS, SJF, RR, AGING (all same output for 1 prog)
