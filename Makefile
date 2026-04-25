.PHONY: up down config

up:
	docker-compose up -d

down:
	docker-compose down

config:
	docker-compose config
