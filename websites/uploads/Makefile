COMPOSE = docker compose -f srcs/docker-compose.yml --env-file srcs/.env
DOMAIN = $(shell grep -E '^DOMAIN_NAME=' srcs/.env | cut -d= -f2)
DATA_DIR = $(HOME)/data

.PHONY: all prepare build up down restart ps logs clean fclean re hosts cert-install

all: prepare build up

prepare:
	@echo "Preparing host data directories at $(DATA_DIR)"
	mkdir -p $(DATA_DIR)/mariadb \
	         $(DATA_DIR)/wordpress \
	         $(DATA_DIR)/redis \
	         $(DATA_DIR)/ftp \
	         $(DATA_DIR)/portainer

	@echo "Setting correct ownership per service"

	# MariaDB 
	@chown -R 999:999 $(DATA_DIR)/mariadb 2>/dev/null || true

	# Redis
	@chown -R 100:101 $(DATA_DIR)/redis 2>/dev/null || true

	# FTP 
	mkdir -p $(DATA_DIR)/ftp
	@chown -R 1000:1000 $(DATA_DIR)/ftp 2>/dev/null || true

	# WordPress + Portainer 
	@chown -R $(shell id -u):$(shell id -g) $(DATA_DIR)/wordpress 2>/dev/null || true
	@chown -R $(shell id -u):$(shell id -g) $(DATA_DIR)/portainer 2>/dev/null || true
	

build:
	@echo "Building images"
	$(COMPOSE) build

up:
	@echo "Starting stack"
	$(COMPOSE) up -d --build

down:
	@echo "Stopping stack"
	$(COMPOSE) down

ps:
	$(COMPOSE) ps

logs:
	$(COMPOSE) logs --tail=200 -f

clean: down
	-@docker volume rm srcs_wordpress_data srcs_mariadb_data || true

fclean: clean
	-@docker rmi srcs-mariadb srcs-wordpress srcs-nginx || true

re: fclean all

hosts:
	@echo "Adding $(DOMAIN) to /etc/hosts (requires sudo)"
	@grep -q "$(DOMAIN)" /etc/hosts 2>/dev/null || sudo sh -c 'printf "127.0.0.1 $(DOMAIN)\n" >> /etc/hosts'

# Copy nginx cert from container and install to system (requires sudo)
cert-install:
	@echo "Copying cert from nginx container to /tmp/maelmahf.crt"
	@docker cp nginx:/etc/ssl/certs/server.crt /tmp/maelmahf.crt || (echo "failed to copy cert"; exit 1)
	@echo "Installing certificate to system trust store (requires sudo)"
	@sudo cp /tmp/maelmahf.crt /usr/local/share/ca-certificates/maelmahf.crt && sudo update-ca-certificates || (echo "failed to install cert"; exit 1)