Feature: `--count=N` option

  Use the `--count=N` option to exit after receiving N packets.

  We're listening on the lo interface which receives all packets in both
  directions. Since we're sending 3 echo requests and we expect 3 echo replies,
  we'd normally see 6 packets when collecting both rx and tx. However, on the
  lo we should see 12.

  We want to make sure we test a value lower than this, so we don't have a
  false positive.

  Scenario: With `--count=6`
    When I run `sudo ../../rxtxcpu --count 6 lo` in background
    And I run `taskset -c 0 ping -c3 localhost`
    Then the output from "sudo ../../rxtxcpu --count 6 lo" should contain exactly:
    """
    6 packets captured on cpu0.
    0 packets captured on cpu1.
    6 packets captured total.
    """

  Scenario: With `-c6`
    When I run `sudo ../../rxtxcpu -c6 lo` in background
    And I run `taskset -c 0 ping -c3 localhost`
    Then the output from "sudo ../../rxtxcpu -c6 lo" should contain exactly:
    """
    6 packets captured on cpu0.
    0 packets captured on cpu1.
    6 packets captured total.
    """

  Scenario: With `--count=0`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 ../../rxtxcpu --count=0 lo` in background
    And I run `taskset -c 0 ping -c3 localhost`
    Then the output from "sudo timeout -s INT 5 ../../rxtxcpu --count=0 lo" should contain exactly:
    """
    12 packets captured on cpu0.
    0 packets captured on cpu1.
    12 packets captured total.
    """

  Scenario: Without `--count`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 ../../rxtxcpu lo` in background
    And I run `taskset -c 0 ping -c3 localhost`
    Then the output from "sudo timeout -s INT 5 ../../rxtxcpu lo" should contain exactly:
    """
    12 packets captured on cpu0.
    0 packets captured on cpu1.
    12 packets captured total.
    """
