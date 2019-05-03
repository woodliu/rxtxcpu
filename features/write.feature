Feature: `--write=FILE`

  Use the `--write=FILE` option to write per-core pcap files.

  Scenario: With `--write=out.pcap`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 ../../rxtxcpu --write out.pcap -U lo` in background
    And I run `taskset -c 0 ping -c3 localhost`
    And I run `tcpdump -r out-0.pcap`
    And I run `tcpdump -r out-1.pcap`
    Then the output from "sudo timeout -s INT 5 ../../rxtxcpu --write out.pcap -U lo" should contain exactly:
    """
    12 packets captured on cpu0.
    0 packets captured on cpu1.
    12 packets captured total.
    """
    And the output from "tcpdump -r out-0.pcap" should contain "IP localhost > localhost: ICMP echo request"
    And the output from "tcpdump -r out-1.pcap" should not contain "IP localhost > localhost: ICMP echo request"

  Scenario: With `-w out.pcap`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 ../../rxtxcpu -w out.pcap -U lo` in background
    And I run `taskset -c 0 ping -c3 localhost`
    And I run `tcpdump -r out-0.pcap`
    And I run `tcpdump -r out-1.pcap`
    Then the output from "sudo timeout -s INT 5 ../../rxtxcpu -w out.pcap -U lo" should contain exactly:
    """
    12 packets captured on cpu0.
    0 packets captured on cpu1.
    12 packets captured total.
    """
    And the output from "tcpdump -r out-0.pcap" should contain "IP localhost > localhost: ICMP echo request"
    And the output from "tcpdump -r out-1.pcap" should not contain "IP localhost > localhost: ICMP echo request"

  Scenario: With `-w -` with a single cpu.
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 bash -c '../../rxtxcpu -l 0 -w - -U lo | tcpdump -c 3 -r -'` in background
    And I run `taskset -c 0 ping -c3 localhost`
    Then the output from "sudo timeout -s INT 5 bash -c '../../rxtxcpu -l 0 -w - -U lo | tcpdump -c 3 -r -'" should contain "IP localhost > localhost: ICMP echo request"
