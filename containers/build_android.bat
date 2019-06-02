@echo off

vagrant up
vagrant ssh -C "singularity run -B /vagrant-showtime-src:/showtime-source,/vagrant-showtime-build:/showtime-output /VMs/build-android.sif"
vagrant halt
