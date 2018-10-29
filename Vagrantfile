Vagrant.configure("2") do |config|

  config.vm.define "seven", primary: true do |seven|
    seven.vm.box = "centos/7"
    seven.vm.provider "virtualbox" do |v|
      v.memory = 4096
      v.cpus = 2
    end
    seven.vm.provision "shell", path: "provisioner.sh"
    seven.vm.synced_folder ".", "/vagrant", type: "virtualbox"
  end

  config.vm.define "trusty", autostart: false do |trusty|
    trusty.vm.box = "ubuntu/trusty64"
    trusty.vm.provider "virtualbox" do |v|
      v.memory = 4096
      v.cpus = 2
    end
    trusty.vm.provision "shell", path: "provisioner.sh"
    trusty.vm.synced_folder ".", "/vagrant", type: "rsync"
    trusty.vbguest.auto_update = false
  end

  config.vm.define "xenial", autostart: false do |xenial|
    xenial.vm.box = "ubuntu/xenial64"
    xenial.vm.provider "virtualbox" do |v|
      v.memory = 4096
      v.cpus = 2
    end
    xenial.vm.provision "shell", path: "provisioner.sh"
    xenial.vm.synced_folder ".", "/vagrant", type: "rsync"
    xenial.vbguest.auto_update = false
  end

  config.vm.define "bionic", autostart: false do |bionic|
    bionic.vm.box = "ubuntu/bionic64"
    bionic.vm.provider "virtualbox" do |v|
      v.memory = 4096
      v.cpus = 2
    end
    bionic.vm.provision "shell", path: "provisioner.sh"
    bionic.vm.synced_folder ".", "/vagrant", type: "rsync"
    bionic.vbguest.auto_update = false
  end
end
