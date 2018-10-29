Feature: `--cpu-list=CPULIST`

  Use the `--cpu-list=CPULIST` option to capture on a subset of cores.

  Scenario: With `--cpu-list=0`
    Given I wait 2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 ../../rxtxcpu --cpu-list 0 lo` in background
    And I run `taskset -c 0 ping -c2 localhost`
    And I run `taskset -c 1 ping -c1 localhost`
    Then the output from "sudo timeout -s INT 5 ../../rxtxcpu --cpu-list 0 lo" should contain exactly:
    """
    8 packets captured on cpu0.
    8 packets captured total.
    """

  Scenario: With `-l0`
    Given I wait 2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 ../../rxtxcpu -l0 lo` in background
    And I run `taskset -c 0 ping -c2 localhost`
    And I run `taskset -c 1 ping -c1 localhost`
    Then the output from "sudo timeout -s INT 5 ../../rxtxcpu -l0 lo" should contain exactly:
    """
    8 packets captured on cpu0.
    8 packets captured total.
    """
