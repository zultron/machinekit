FROM debian
MAINTAINER John Morris <john@zultron.com>

######################################
# Configuration

# Configure the github user/repo/branch
ENV github_user zultron
ENV github_repo machinekit
ENV github_branch docker

# Configure the OBS repo
# URL format example:
#   http://download.opensuse.org/repositories/home:/zultron:/test/Debian_7.0/
ENV obs_user zultron
ENV obs_project machinekit-deps
ENV obs_distro Debian_7.0

# Alternate Debian server; http.debian.net is default in `debian` Docker img
ENV debian_server ftp.us.debian.org

######################################
# Misc

# Silence annoying warnings
RUN	dpkg-reconfigure --frontend=teletype --priority=critical debconf

######################################
# Apt configuration

# Set Debian package server
RUN	sed -i "s@http://[^/ ]*/@http://${debian_server}/@" \
	    /etc/apt/sources.list

# Install basic build deps
RUN	apt-get update
RUN	apt-get install -y --no-install-recommends \
	    wget \
	    devscripts \
	    fakeroot \
	    equivs \
	    lsb-release \
	    libfile-fcntllock-perl

# Install Cython from wheezy-backports
RUN	echo "deb http://${debian_server}/debian wheezy-backports main" \
	    > /etc/apt/sources.list.d/wheezy-backports.list
RUN	apt-get update
RUN	apt-get install -y -t wheezy-backports cython

# Disabled:  the OBS packages are newer
# # Set up Dovetail Automata Debian package repo
# RUN	apt-key adv --keyserver hkp://keys.gnupg.net --recv-key 73571BB9
# RUN	echo "deb http://deb.dovetail-automata.com wheezy main" \
# 	    > /etc/apt/sources.list.d/machinekit.list

# Set up OBS Debian package repo
ENV obs_url_base http://download.opensuse.org/repositories/home:
ENV obs_url ${obs_url_base}/${obs_user}:/${obs_project}/${obs_distro}
RUN	wget -O - -q ${obs_url}/Release.key | apt-key add -
RUN	echo deb ${obs_url}/ ./ > \
	    /etc/apt/sources.list.d/${obs_user}-obs.list
RUN	apt-get update

######################################
# Source tree and build deps

# Unpack source tree
RUN mkdir /usr/src/machinekit
WORKDIR /usr/src/machinekit
ENV github_base_url https://github.com/${github_user}/${github_repo}
RUN	wget ${github_base_url}/archive/${github_branch}.tar.gz \
		-q -O - --no-check-certificate | \
	    tar xzf - --strip-components=1

# Configure source package
RUN	./debian/configure -px  # resin.io:  Only Xenomai + POSIX threads

# Install build deps
RUN	yes | mk-build-deps -i -r

######################################
# Build

RUN	debuild -eDEB_BUILD_OPTIONS="parallel=4" -us -uc -b -j4
