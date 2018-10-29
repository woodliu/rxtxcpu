Feature: any interface

  Scenario: Omitting interface is valid
    When I run `sudo ../../rxtxcpu -c6 -l0` in background
    And I run `taskset -c 0 ping -c3 localhost`
    Then the stdout from "sudo ../../rxtxcpu -c6 -l0" should contain exactly:
    """
    6 packets captured on cpu0.
    6 packets captured total.
    """
