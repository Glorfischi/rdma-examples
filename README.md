## RDMA Examples

This repository contains code snippets I wrote while learning to use RDMA using `ibverbs`. This is in no way polished, idiomatic,
or efficient, but it might help you to get started too.


### Structure

* `src/` contains all code snippets. Each of them can be built using `make` and contains a `play.yml` (See [Test Environment](#test-enviroment)).
* `roles/`, `host_vars/`, `inv/` are all ansible and test enviroment specific.

#### Test Environment

I set up a very simple test environment using vagrant and ansible. This gives you two virtual machines with all necessary 
libraries installed and both having a infiniband HCA using software RoCE. 

To use this test enviroment you need to

* Install [Ansible](https://docs.ansible.com/ansible/latest/installation_guide/intro_installation.html), [Vagant](https://www.vagrantup.com/intro/getting-started/install.html),
and either libvirt or virtualbox. (Note: This is only tested using libvirt)
* Install necessary libraries locally to compile it
  - `ibverbs`
  - `rdmacm`
* Run `vagrant up`
* Run `ansible-playbook src/<snippet>/play.yml`

You can now ssh to either of the two virtual machines using the user `vagrant`

```
   ssh vagrant@172.17.5.101
```

The contain the binaries in the home directory.
