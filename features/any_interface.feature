Feature: any interface

  Scenario: Omitting interface is valid
    When I run `sudo ../../rxtxcpu -c6 -l0` in background
    And I run `ping -c3 localhost` on cpu 0
    Then the stdout from "sudo ../../rxtxcpu -c6 -l0" should contain exactly:
    """
    6 packets captured on cpu0.
    6 packets captured total.
    """
