Feature: Cli

  Scenario: invalid count
    When I run `./rxtxcpu -c 10j`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: Invalid count '10j'."
    And the stderr should contain "Usage: rxtxcpu [--help]"

  Scenario: invalid cpu list
    When I run `./rxtxcpu -l 10j`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: Invalid cpu list '10j'."
    And the stderr should contain "Usage: rxtxcpu [--help]"

  Scenario: invalid cpu mask
    When I run `./rxtxcpu -m 10j`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: Invalid cpu mask '10j'."
    And the stderr should contain "Usage: rxtxcpu [--help]"

  Scenario: invalid direction
    When I run `./rxtxcpu -d 10j`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: Invalid direction '10j'."
    And the stderr should contain "Usage: rxtxcpu [--help]"

  Scenario: write file argument '-' with more than one cpu
    When I run `./rxtxcpu -w -`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: Write file '-' (stdout) is only permitted when capturing on a single cpu."
    And the stderr should contain "Usage: rxtxcpu [--help]"

  Scenario: missing write file argument
    When I run `./rxtxcpu -w`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: Option '-w' requires an argument."
    And the stderr should contain "Usage: rxtxcpu [--help]"

  Scenario: unrecognized argument
    When I run `./rxtxcpu -z`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: Unrecognized option '-z'."
    And the stderr should contain "Usage: rxtxcpu [--help]"

  Scenario: unrecognized bundled argument
    When I run `./rxtxcpu -zv`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: Unrecognized option '-z'."
    And the stderr should contain "Usage: rxtxcpu [--help]"

  Scenario: cpu list and cpu mask
    When I run `./rxtxcpu -l0 -m1`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: -l [--cpu-list] and -m [--cpu-mask] are mutually exclusive."
    And the stderr should contain "Usage: rxtxcpu [--help]"

  Scenario: cpu list with no configured cpus
    When I run `./rxtxcpu -l10`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: No configured cpus present in cpu list."
    And the stderr should contain "Usage: rxtxcpu [--help]"

  Scenario: cpu mask with no configured cpus
    When I run `./rxtxcpu -m200`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: No configured cpus present in cpu mask."
    And the stderr should contain "Usage: rxtxcpu [--help]"

  Scenario: cpu list with no online cpus
    # The following is equivalent to `sudo chcpu -d 1`
    When I run `bash -c 'echo 0 | sudo tee /sys/devices/system/node/node0/cpu1/online'`
    And I run `./rxtxcpu -l1`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: No online cpus present in cpu list."
    And the stderr should contain "Usage: rxtxcpu [--help]"
    # The following is equivalent to `sudo chcpu -e 1`
    And I run `bash -c 'echo 1 | sudo tee /sys/devices/system/node/node0/cpu1/online'`

  Scenario: cpu mask with no online cpus
    # The following is equivalent to `sudo chcpu -d 1`
    When I run `bash -c 'echo 0 | sudo tee /sys/devices/system/node/node0/cpu1/online'`
    And I run `./rxtxcpu -m2`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: No online cpus present in cpu mask."
    And the stderr should contain "Usage: rxtxcpu [--help]"
    # The following is equivalent to `sudo chcpu -e 1`
    And I run `bash -c 'echo 1 | sudo tee /sys/devices/system/node/node0/cpu1/online'`

  Scenario: more than one interface
    When I run `./rxtxcpu lo lo2`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: Only one interface argument is allowed (got"
    And the stderr should contain "Usage: rxtxcpu [--help]"
