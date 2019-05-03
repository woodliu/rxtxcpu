Feature: fanout_group

  Scenario: fanout_group_id is properly set
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 7 hold-fanout-group-id-zero` in background
    And I run `sudo timeout -s INT 5 ../../rxtxcpu lo` in background
    And I run `ping -i0.2 -c2 localhost` on cpu 0
    Then the output from "sudo timeout -s INT 5 ../../rxtxcpu lo" should contain exactly:
    """
    8 packets captured on cpu0.
    0 packets captured on cpu1.
    8 packets captured total.
    """
