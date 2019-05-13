Feature: affinity

  This is the core functionality of rxtxcpu. We want to ensure the workers are
  capturing the proper packets.

  Scenario: Packets sent on cpu0 are counted as such
    When I run `sudo ../../rxtxcpu -c6 lo` in background
    And I run `ping -i0.2 -c3 localhost` on cpu 0
    Then the stdout from "sudo ../../rxtxcpu -c6 lo" should contain exactly:
    """
    6 packets captured on cpu0.
    0 packets captured on cpu1.
    6 packets captured total.
    """

  Scenario: Packets sent on cpu1 are counted as such
    When I run `sudo ../../rxtxcpu -c6 lo` in background
    And I run `ping -i0.2 -c3 localhost` on cpu 1
    Then the stdout from "sudo ../../rxtxcpu -c6 lo" should contain exactly:
    """
    0 packets captured on cpu0.
    6 packets captured on cpu1.
    6 packets captured total.
    """

  Scenario: Unreliable packets are skipped
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 2 taskset -c 1 ping -i0 127.0.0.1` in background
    And I run `sudo timeout -s INT 2 ../../rxtxcpu lo`
    Then the stdout from "sudo timeout -s INT 2 ../../rxtxcpu lo" should contain "0 packets captured on cpu0."
    And the stdout from "sudo timeout -s INT 2 ../../rxtxcpu lo" should not contain "0 packets captured on cpu1."
