Feature: `--packet-buffered`

  Use the `--packet-buffered` option to ensure the pcap write buffer is flushed
  after each packet.

  Scenario: With `--packet-buffered`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 ../../rxtxcpu -w out.pcap --packet-buffered lo` in background
    And I run `ping -c3 localhost` on cpu 0
    And I run `tcpdump -r out-0.pcap`
    And I run `tcpdump -r out-1.pcap`
    Then the output from "sudo timeout -s INT 5 ../../rxtxcpu -w out.pcap --packet-buffered lo" should contain exactly:
    """
    12 packets captured on cpu0.
    0 packets captured on cpu1.
    12 packets captured total.
    """
    And the output from "tcpdump -r out-0.pcap" should contain "IP localhost > localhost: ICMP echo request"
    And the output from "tcpdump -r out-1.pcap" should not contain "IP localhost > localhost: ICMP echo request"

  Scenario: With `-U`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 ../../rxtxcpu -w out.pcap -U lo` in background
    And I run `ping -c3 localhost` on cpu 0
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

  Scenario: Without `--packet-buffered`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 ../../rxtxcpu -w out.pcap lo` in background
    And I run `ping -c3 localhost` on cpu 0
    And I run `tcpdump -r out-0.pcap`
    And I run `tcpdump -r out-1.pcap`
    Then the output from "sudo timeout -s INT 5 ../../rxtxcpu -w out.pcap lo" should contain exactly:
    """
    12 packets captured on cpu0.
    0 packets captured on cpu1.
    12 packets captured total.
    """
    And the output from "tcpdump -r out-0.pcap" should contain "tcpdump: truncated dump file; tried to read 4 file header bytes, only got 0"
