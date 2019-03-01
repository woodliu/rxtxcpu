Vagrant.configure("2") do |config|

  config.vm.define "seven", primary: true do |_self|
    _self.vm.box = "centos/7"
    _self.vm.provider "virtualbox" do |v|
      v.memory = 4096
      v.cpus = 2
    end
    _self.vm.provision "shell", path: "provisioners/centos/requires.sh"
    _self.vm.synced_folder ".", "/vagrant", type: "virtualbox"
  end

  config.vm.define "trusty", autostart: false do |_self|
    _self.vm.box = "ubuntu/trusty64"
    _self.vm.provider "virtualbox" do |v|
      v.memory = 4096
      v.cpus = 2
    end
    _self.vm.provision "shell", path: "provisioners/ubuntu/requires.sh"
    _self.vm.synced_folder ".", "/vagrant", type: "rsync"
    _self.vbguest.auto_update = false
  end

  config.vm.define "xenial", autostart: false do |_self|
    _self.vm.box = "ubuntu/xenial64"
    _self.vm.provider "virtualbox" do |v|
      v.memory = 4096
      v.cpus = 2
    end
    _self.vm.provision "shell", path: "provisioners/ubuntu/requires.sh"
    _self.vm.synced_folder ".", "/vagrant", type: "rsync"
    _self.vbguest.auto_update = false
  end

  config.vm.define "bionic", autostart: false do |_self|
    _self.vm.box = "ubuntu/bionic64"
    _self.vm.provider "virtualbox" do |v|
      v.memory = 4096
      v.cpus = 2
    end
    _self.vm.provision "shell", path: "provisioners/ubuntu/requires.sh"
    _self.vm.synced_folder ".", "/vagrant", type: "rsync"
    _self.vbguest.auto_update = false
  end
end
