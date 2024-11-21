package main

import (
	"context"
	"flag"
	"fmt"
	"os"
	"strconv"
	"strings"

	influxdb2 "github.com/influxdata/influxdb-client-go/v2"
)

const (
	influxDBHost   = "http://192.168.1.88:8086"
	influxDBToken  = "RHl3fYEp8eMLtIUraVPzY4zp_hnnu2kYlR9hYrUaJLcq5mB2PvDsOi9SR0Tu_i-t_183fHb1a95BTJug-vAPVQ=="
	influxDBOrg    = "Milano"
	influxDBBucket = "MWPWater"
)

var timeWindows = map[string]string{
	"1m":   "1m",
	"5m":   "5m",
	"1h":   "1h",
	"12h":  "12h",
	"24h":  "24h",
	"7d":   "168h",
	"30d":  "720h",
	"3mo":  "2160h",
	"6mo":  "4320h",
	"12mo": "8760h",
}

func buildQuery(bucket, timeRange, controller, zone string) string {
	return fmt.Sprintf(`
        from(bucket: "%s")
            |> range(start: -%s)
            |> filter(fn: (r) => r["_measurement"] == "mwp_sensors")
            |> filter(fn: (r) => r["Controller"] == "%s" and r["Zone"] == "%s")
    `, bucket, timeRange, controller, zone)
}

func main() {
	// Command line flags
	rangeFlag := flag.String("range", "1h", "Time range to query")
	flag.StringVar(rangeFlag, "r", "1h", "Time range (shorthand)")
	controllerFlag := flag.String("controller", "3", "Controller number (1-5)")
	flag.StringVar(controllerFlag, "c", "3", "Controller (shorthand)")
	zoneFlag := flag.String("zone", "1", "Zone number (1-16)")
	flag.StringVar(zoneFlag, "z", "1", "Zone (shorthand)")
	rawFlag := flag.Bool("raw", false, "Show all fields including internal ones")
	flag.Parse()

	// Validate time range
	if _, ok := timeWindows[*rangeFlag]; !ok {
		fmt.Printf("Invalid time range. Valid options are: %s\n", strings.Join(keys(timeWindows), ", "))
		os.Exit(1)
	}

	// Validate controller and zone
	controllerNum, err := strconv.Atoi(*controllerFlag)
	if err != nil || controllerNum < 1 || controllerNum > 5 {
		fmt.Println("Invalid controller. Must be between 1 and 5")
		os.Exit(1)
	}

	zoneNum, err := strconv.Atoi(*zoneFlag)
	if err != nil || zoneNum < 1 || zoneNum > 16 {
		fmt.Println("Invalid zone. Must be between 1 and 16")
		os.Exit(1)
	}

	// Configure client
	client := influxdb2.NewClient(influxDBHost, influxDBToken)
	defer client.Close()
	queryAPI := client.QueryAPI(influxDBOrg)

	query := buildQuery(influxDBBucket, timeWindows[*rangeFlag], *controllerFlag, *zoneFlag)
	result, err := queryAPI.Query(context.Background(), query)
	if err != nil {
		fmt.Printf("Query error: %s\n", err)
		os.Exit(1)
	}

	fmt.Printf("\nRAW DATA - Controller: %s, Zone: %s, Range: %s\n\n",
		*controllerFlag, *zoneFlag, *rangeFlag)

	if *rawFlag {
		// Show everything except excluded fields
		for result.Next() {
			record := result.Record()
			fmt.Printf("Record at %s:\n", record.Time().Format("2006-01-02 15:04:05"))
			for k, v := range record.Values() {
				// Skip excluded fields
				if k != "_measurement" && k != "host" && k != "topic" {
					fmt.Printf("  %-20s = %v\n", k, v)
				}
			}
			fmt.Println()
		}
	} else {
		// Show formatted data without excluded fields
		fmt.Println("Time                     Field         Value    Controller  Zone")
		fmt.Println("------------------------------------------------------------")
		for result.Next() {
			record := result.Record()
			fmt.Printf("%s  %-12s  %-8v  %-11s  %s\n",
				record.Time().Format("2006-01-02 15:04:05"),
				record.Field(),
				record.Value(),
				record.ValueByKey("Controller"),
				record.ValueByKey("Zone"))
		}
	}

	if result.Err() != nil {
		fmt.Printf("Query result error: %s\n", result.Err())
		os.Exit(1)
	}
}

func keys(m map[string]string) []string {
	keys := make([]string, 0, len(m))
	for k := range m {
		keys = append(keys, k)
	}
	return keys
}
