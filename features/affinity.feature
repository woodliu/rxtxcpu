Feature: affinity

  This is the core functionality of rxtxcpu. We want to ensure the workers are
  capturing the proper packets.

  Scenario: Packets sent on cpu0 are counted as such
    When I run `sudo ../../rxtxcpu -c6 lo` in background
    And I run `ping -c3 localhost` on cpu 0
    Then the stdout from "sudo ../../rxtxcpu -c6 lo" should contain exactly:
    """
    6 packets captured on cpu0.
    0 packets captured on cpu1.
    6 packets captured total.
    """

  Scenario: Packets sent on cpu1 are counted as such
    When I run `sudo ../../rxtxcpu -c6 lo` in background
    And I run `ping -c3 localhost` on cpu 1
    Then the stdout from "sudo ../../rxtxcpu -c6 lo" should contain exactly:
    """
    0 packets captured on cpu0.
    6 packets captured on cpu1.
    6 packets captured total.
    """
