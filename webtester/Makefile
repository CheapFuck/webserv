VENV_NAME = env
PYTHON = python3
ACTIVATE = . $(VENV_NAME)/bin/activate
ENAME = main.py
DRIVERURL = https://github.com/mozilla/geckodriver/releases/download/v0.36.0/geckodriver-v0.36.0-linux64.tar.gz
DRIVER_ARCHIVE = geckodriver-v0.36.0-linux64.tar.gz
DRIVER_DIR = .
DRIVER_BIN = $(DRIVER_DIR)/geckodriver
TMPDIR = "tmp"

.PHONY: all env install clean download_driver extract_driver run test_folders_make

all: env install download_driver extract_driver

run: 
	tmux new-session -d -s testsession \; \
		split-window -h\; \
		select-pane -t 0 \; \
		send-keys 'cd ../ && make && ./webserv webtester/testconf.conf' C-m\; \
		select-pane -t 1 \; \
		send-keys '$(ACTIVATE) && $(PYTHON) $(ENAME)' C-m\; \
		attach-session
	
env:
	mkdir -p $(TMPDIR)
	$(PYTHON) -m venv $(VENV_NAME)

install: env 
	$(ACTIVATE) && pip install --upgrade pip
	$(ACTIVATE) && pip install selenium webdriver-manager

download_driver:
	if [ ! -f $(DRIVER_ARCHIVE) ]; then \
		wget -O $(DRIVER_ARCHIVE) $(DRIVERURL); \
	else \
		echo "$(DRIVER_ARCHIVE) already exists, skipping download."; \
	fi

extract_driver: download_driver
	tar -xvf $(DRIVER_ARCHIVE) -C $(DRIVER_DIR)
	chmod +x $(DRIVER_BIN)

clean:
	rm -rf $(VENV_NAME) $(DRIVER_BIN) $(DRIVER_ARCHIVE) $(TMPDIR)
	rm -rf vartest/
 
test_folders_make:
	mkdir -p vartest/www/get
	echo '<html><head><title>get</title></head><body>GET page OH THE BODY</body></html>' > vartest/www/get/index.html

	mkdir -p vartest/www/post/new
	echo '<html><head><title>post new</title></head><body>POST new page</body></html>' > vartest/www/post/new/index.html

	mkdir -p vartest/www/post/create
	echo '<html><body>created</body></html>' > vartest/www/post/create/index.html

	mkdir -p vartest/www/put/1/edit
	echo '<html><head><title>put edit</title></head><body>PUT edit page</body></html>' > vartest/www/put/1/edit/index.html

	mkdir -p vartest/www/put/1/update
	echo '<html><body>updated</body></html>' > vartest/www/put/1/update/index.html

	mkdir -p vartest/www/delete/1
	echo '<html><head><title>delete</title></head><body>Delete page</body></html>' > vartest/www/delete/1/index.html

	mkdir -p vartest/www/delete/1/confirm
	echo '<html><body>deleted</body></html>' > vartest/www/delete/1/confirm/index.html
	chmod -R 777 vartest
