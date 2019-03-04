#!/bin/bash

set -e

. <(grep '^ID=' /etc/os-release | sed 's/^/OS_RELEASE_/')

rvm --version 2>/dev/null || {
  gpg --keyserver hkp://keys.gnupg.net \
      --recv-keys 409B6B1796C275462A1703113804BB82D39DC0E3 \
                  7D2BAF1CF37B13E2069D6956105BD0E739499BDB
  curl -sSL https://get.rvm.io | bash -s stable
}

set +e
. "$HOME"/.rvm/scripts/rvm
set -e

ruby_version="2.3.4"

rvm list strings | grep -q "^ruby-${ruby_version}\$" || {
  rvm install "$ruby_version"
  rvm use "$ruby_version" --default
}

gem install --conservative bundler

bundle install
[[ $OS_RELEASE_ID == "centos" ]] &&
  bundle exec cucumber --tags 'not @ExcludeFromCentOS'
[[ $OS_RELEASE_ID == "ubuntu" ]] &&
  bundle exec cucumber --tags 'not @ExcludeFromUbuntu'
