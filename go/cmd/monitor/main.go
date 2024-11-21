package main

import (
	"context"
	"flag"
	"fmt"
	"log"
	"os"
	"os/signal"
	"syscall"
	"time"
	
	MQTT "github.com/eclipse/paho.mqtt.golang"
)

// Config holds the application configuration
type Config struct {
	mqttBroker   string
	mqttPort     int
	mqttClientID string
	mqttTopic    string
	debugMode    bool
}

func main() {
	// Initialize configuration
	cfg := parseFlags()

	// Set up logging
	log := log.New(os.Stdout, "[MONITOR] ", log.LstdFlags)
	
	// Create a context that can be cancelled
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Set up signal handling for graceful shutdown
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	// Initialize MQTT client
	client, err := setupMQTTClient(cfg)
	if err != nil {
		log.Fatalf("Failed to setup MQTT client: %v", err)
	}
	defer client.Disconnect(250)

	// Start the monitor service
	errChan := make(chan error, 1)
	go runMonitor(ctx, client, cfg, log, errChan)

	// Wait for shutdown signal or error
	select {
	case sig := <-sigChan:
		log.Printf("Received signal: %v", sig)
	case err := <-errChan:
		log.Printf("Error in monitor: %v", err)
	}

	// Graceful shutdown
	log.Println("Shutting down...")
	shutdownCtx, shutdownCancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer shutdownCancel()

	// Perform cleanup
	if err := cleanup(shutdownCtx); err != nil {
		log.Printf("Error during cleanup: %v", err)
	}
}

func parseFlags() *Config {
	cfg := &Config{}
	
	flag.StringVar(&cfg.mqttBroker, "broker", "localhost", "MQTT broker address")
	flag.IntVar(&cfg.mqttPort, "port", 1883, "MQTT broker port")
	flag.StringVar(&cfg.mqttClientID, "client-id", "go-monitor", "MQTT client ID")
	flag.StringVar(&cfg.mqttTopic, "topic", "monitor/#", "MQTT topic to subscribe to")
	flag.BoolVar(&cfg.debugMode, "debug", false, "Enable debug mode")
	
	flag.Parse()
	return cfg
}

func setupMQTTClient(cfg *Config) (MQTT.Client, error) {
	opts := MQTT.NewClientOptions()
	broker := fmt.Sprintf("tcp://%s:%d", cfg.mqttBroker, cfg.mqttPort)
	opts.AddBroker(broker)
	opts.SetClientID(cfg.mqttClientID)
	
	client := MQTT.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		return nil, token.Error()
	}
	
	return client, nil
}

func runMonitor(ctx context.Context, client MQTT.Client, cfg *Config, log *log.Logger, errChan chan<- error) {
	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			if cfg.debugMode {
				log.Println("Monitor tick")
			}
		}
	}
}

func cleanup(ctx context.Context) error {
	return nil
}