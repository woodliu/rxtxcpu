Feature: `--cpu-mask=CPUMASK`

  Use the `--cpu-mask=CPUMASK` option to capture on a subset of cores.

  Scenario: With `--cpu-mask=1`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 ../../rxtxcpu --cpu-mask 1 lo` in background
    And I run `ping -i0.2 -c2 localhost` on cpu 0
    And I run `ping -i0.2 -c1 localhost` on cpu 1
    Then the output from "sudo timeout -s INT 5 ../../rxtxcpu --cpu-mask 1 lo" should contain exactly:
    """
    8 packets captured on cpu0.
    8 packets captured total.
    """

  Scenario: With `-m1`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 ../../rxtxcpu -m1 lo` in background
    And I run `ping -i0.2 -c2 localhost` on cpu 0
    And I run `ping -i0.2 -c1 localhost` on cpu 1
    Then the output from "sudo timeout -s INT 5 ../../rxtxcpu -m1 lo" should contain exactly:
    """
    8 packets captured on cpu0.
    8 packets captured total.
    """
