# Deployment Guide

## Overview

This guide covers deployment strategies, operational considerations, and production setup for LKJAgent. It includes containerized deployment, configuration management, monitoring, and troubleshooting procedures.

## Deployment Options

### 1. Standalone Binary Deployment

#### Building Production Binary

```bash
# Build optimized binary
make clean && make

# Create deployment package
tar -czf lkjagent-deployment.tar.gz \
    build/lkjagent \
    data/config.json \
    data/memory.json \
    docs/

# Transfer to target system
scp lkjagent-deployment.tar.gz user@target:/opt/lkjagent/
```

#### System Requirements

**Minimum:**
- Linux x86_64 with kernel 3.2+
- 512MB RAM (for basic operation)
- 50MB disk space
- Network access to LLM endpoint

**Recommended:**
- 2GB+ RAM (for optimal pool performance)
- SSD storage for faster I/O
- Reliable network connection
- Monitoring and logging infrastructure

#### Installation Steps

```bash
# On target system
cd /opt/lkjagent
tar -xzf lkjagent-deployment.tar.gz

# Set permissions
chmod +x build/lkjagent
chmod 600 data/*.json

# Test installation
./build/lkjagent
```

### 2. Docker Deployment

#### Production Dockerfile

```dockerfile
# Multi-stage build for minimal production image
FROM gcc:12 as builder

# Copy source code
COPY ./src/ /app/src/
COPY ./Makefile /app/

# Build application
WORKDIR /app
RUN make

# Production stage - minimal image
FROM scratch

# Copy binary and set as entrypoint
COPY --from=builder /app/build/lkjagent /app/lkjagent

# Set working directory
WORKDIR /app

# Define entrypoint
ENTRYPOINT ["/app/lkjagent"]
```

#### Building Production Image

```bash
# Build image with version tag
docker build -t lkjagent:1.0.0 .
docker tag lkjagent:1.0.0 lkjagent:latest

# Push to registry (if using container registry)
docker push your-registry/lkjagent:1.0.0
```

#### Container Deployment

```bash
# Create data directory
mkdir -p /opt/lkjagent/data

# Copy configuration files
cp config.json /opt/lkjagent/data/
cp memory.json /opt/lkjagent/data/

# Run container
docker run -d \
  --name lkjagent \
  --restart unless-stopped \
  -v /opt/lkjagent/data:/app/data:rw \
  -e LLM_ENDPOINT=http://your-llm-server:1234/v1/chat/completions \
  lkjagent:1.0.0
```

### 3. Docker Compose Deployment

#### Production docker-compose.yml

```yaml
version: '3.8'

services:
  lkjagent:
    image: lkjagent:1.0.0
    container_name: lkjagent
    restart: unless-stopped
    volumes:
      - ./data:/app/data:rw
      - ./logs:/app/logs:rw
    environment:
      - LLM_ENDPOINT=${LLM_ENDPOINT}
      - LOG_LEVEL=${LOG_LEVEL:-INFO}
    networks:
      - lkjagent-network
    depends_on:
      - llm-server
    healthcheck:
      test: ["CMD", "/app/lkjagent", "--health-check"]
      interval: 30s
      timeout: 10s
      retries: 3

  llm-server:
    image: ollama/ollama:latest
    container_name: llm-server
    restart: unless-stopped
    ports:
      - "11434:11434"
    volumes:
      - ollama-data:/root/.ollama
    environment:
      - OLLAMA_HOST=0.0.0.0
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:11434/api/health"]
      interval: 30s
      timeout: 10s
      retries: 3

  monitoring:
    image: prom/prometheus:latest
    container_name: monitoring
    restart: unless-stopped
    ports:
      - "9090:9090"
    volumes:
      - ./monitoring/prometheus.yml:/etc/prometheus/prometheus.yml
      - prometheus-data:/prometheus

volumes:
  ollama-data:
  prometheus-data:

networks:
  lkjagent-network:
    driver: bridge
```

#### Environment Configuration

```bash
# .env file for docker-compose
LLM_ENDPOINT=http://llm-server:11434/v1/chat/completions
LOG_LEVEL=INFO
RESTART_POLICY=unless-stopped
```

#### Deployment Commands

```bash
# Deploy the stack
docker-compose up -d

# View logs
docker-compose logs -f lkjagent

# Update configuration
docker-compose restart lkjagent

# Scale (if supporting multiple instances)
docker-compose up -d --scale lkjagent=3

# Shutdown
docker-compose down
```

## Configuration Management

### Production Configuration Template

```json
{
  "version": "1.0.0",
  "llm": {
    "endpoint": "${LLM_ENDPOINT}",
    "model": "${LLM_MODEL:-gpt-3.5-turbo}",
    "temperature": 0.7,
    "timeout": 30,
    "max_retries": 3
  },
  "agent": {
    "thinking_log": {
      "enable": true,
      "max_entries": 10,
      "key_prefix": "thinking_log_"
    },
    "paging_limit": {
      "enable": true,
      "max_tokens": 4096
    },
    "hard_limit": {
      "enable": true,
      "max_tokens": 8192
    },
    "iterate": {
      "enable": true,
      "max_iterations": 1000
    },
    "persistence": {
      "backup_interval": 300,
      "backup_count": 10,
      "auto_save": true
    }
  },
  "logging": {
    "level": "${LOG_LEVEL:-INFO}",
    "file": "/app/logs/lkjagent.log",
    "max_size": "100MB",
    "max_files": 10
  },
  "monitoring": {
    "enable": true,
    "metrics_port": 9091,
    "health_check_port": 8080
  }
}
```

### Environment-Specific Configurations

#### Development Environment

```json
{
  "llm": {
    "endpoint": "http://localhost:1234/v1/chat/completions",
    "model": "llama3-8b-instruct",
    "temperature": 0.3
  },
  "agent": {
    "thinking_log": {
      "max_entries": 50
    },
    "paging_limit": {
      "enable": false
    }
  },
  "logging": {
    "level": "DEBUG"
  }
}
```

#### Staging Environment

```json
{
  "llm": {
    "endpoint": "https://staging-api.openai.com/v1/chat/completions",
    "model": "gpt-3.5-turbo",
    "temperature": 0.7
  },
  "agent": {
    "iterate": {
      "max_iterations": 100
    }
  },
  "logging": {
    "level": "INFO"
  }
}
```

#### Production Environment

```json
{
  "llm": {
    "endpoint": "https://api.openai.com/v1/chat/completions",
    "model": "gpt-4",
    "temperature": 0.7,
    "timeout": 60,
    "max_retries": 5
  },
  "agent": {
    "thinking_log": {
      "max_entries": 10
    },
    "paging_limit": {
      "enable": true,
      "max_tokens": 4096
    },
    "hard_limit": {
      "enable": true,
      "max_tokens": 8192
    }
  },
  "logging": {
    "level": "WARN"
  },
  "monitoring": {
    "enable": true
  }
}
```

## Security Considerations

### API Key Management

#### Environment Variables

```bash
# Set API keys as environment variables
export OPENAI_API_KEY="sk-..."
export LLM_API_KEY="your-api-key"

# In configuration, reference environment variables
"api_key": "${OPENAI_API_KEY}"
```

#### Docker Secrets

```yaml
# docker-compose.yml with secrets
version: '3.8'

services:
  lkjagent:
    image: lkjagent:1.0.0
    secrets:
      - openai_api_key
    environment:
      - OPENAI_API_KEY_FILE=/run/secrets/openai_api_key

secrets:
  openai_api_key:
    file: ./secrets/openai_api_key.txt
```

#### File Permissions

```bash
# Secure configuration files
chmod 600 /opt/lkjagent/data/config.json
chmod 600 /opt/lkjagent/data/memory.json
chown lkjagent:lkjagent /opt/lkjagent/data/*

# Create dedicated user
useradd -r -s /bin/false lkjagent
```

### Network Security

#### Firewall Configuration

```bash
# Allow only necessary ports
ufw allow 22/tcp    # SSH
ufw allow 80/tcp    # HTTP (if needed)
ufw allow 443/tcp   # HTTPS (if needed)
ufw deny 1234/tcp   # Block direct LLM access
ufw --force enable
```

#### TLS/SSL Configuration

```bash
# Use TLS for LLM communication
{
  "llm": {
    "endpoint": "https://secure-llm-endpoint.com/v1/chat/completions",
    "verify_ssl": true,
    "ca_bundle": "/path/to/ca-bundle.crt"
  }
}
```

### Input Validation and Sanitization

```c
// Example input validation (implementation needed)
static result_t validate_llm_response(const string_t* response) {
    // Check response size limits
    if (response->size > MAX_RESPONSE_SIZE) {
        RETURN_ERR("LLM response exceeds size limit");
    }
    
    // Validate XML structure
    if (!is_valid_xml(response)) {
        RETURN_ERR("Invalid XML in LLM response");
    }
    
    // Check for malicious content
    if (contains_suspicious_content(response)) {
        RETURN_ERR("Suspicious content detected in LLM response");
    }
    
    return RESULT_OK;
}
```

## Monitoring and Logging

### System Monitoring

#### Prometheus Metrics Configuration

```yaml
# prometheus.yml
global:
  scrape_interval: 15s

scrape_configs:
  - job_name: 'lkjagent'
    static_configs:
      - targets: ['lkjagent:9091']
    metrics_path: /metrics
    scrape_interval: 10s

  - job_name: 'llm-server'
    static_configs:
      - targets: ['llm-server:9090']
```

#### Key Metrics to Monitor

```c
// Example metrics (implementation needed)
typedef struct {
    uint64_t total_requests;
    uint64_t successful_requests;
    uint64_t failed_requests;
    uint64_t average_response_time_ms;
    uint64_t memory_pool_utilization;
    uint64_t active_sessions;
} agent_metrics_t;
```

**Essential Metrics:**
- Request rate and success rate
- Response time percentiles
- Memory pool utilization
- Error rates by category
- LLM endpoint availability
- Agent state distribution

### Health Checks

#### Application Health Check

```c
// Health check endpoint implementation
result_t health_check(void) {
    // Check memory pools
    if (check_memory_pools() != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Check LLM connectivity
    if (check_llm_endpoint() != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Check configuration validity
    if (validate_configuration() != RESULT_OK) {
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}
```

#### Docker Health Check

```dockerfile
# Add to Dockerfile
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
  CMD ["/app/lkjagent", "--health-check"] || exit 1
```

### Logging Configuration

#### Structured Logging

```c
// Logging levels and structured format
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3
} log_level_t;

// Example structured log entry
{
  "timestamp": "2024-01-15T10:30:00.000Z",
  "level": "INFO",
  "component": "agent.core",
  "message": "Agent cycle completed successfully",
  "metadata": {
    "cycle_id": "12345",
    "state": "thinking",
    "duration_ms": 1250,
    "memory_usage": {
      "working_memory_items": 5,
      "storage_items": 23
    }
  }
}
```

#### Log Rotation

```bash
# logrotate configuration
/opt/lkjagent/logs/*.log {
    daily
    rotate 30
    compress
    delaycompress
    missingok
    notifempty
    postrotate
        docker kill -s USR1 lkjagent
    endscript
}
```

## Backup and Recovery

### Data Backup Strategy

#### Configuration Backup

```bash
#!/bin/bash
# backup-config.sh

BACKUP_DIR="/opt/lkjagent/backups"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Create backup directory
mkdir -p "$BACKUP_DIR"

# Backup configuration and memory
tar -czf "$BACKUP_DIR/lkjagent-config-$TIMESTAMP.tar.gz" \
    /opt/lkjagent/data/config.json \
    /opt/lkjagent/data/memory.json

# Keep only last 30 backups
find "$BACKUP_DIR" -name "lkjagent-config-*.tar.gz" -mtime +30 -delete

echo "Backup completed: lkjagent-config-$TIMESTAMP.tar.gz"
```

#### Automated Backup with Cron

```bash
# Add to crontab
0 2 * * * /opt/lkjagent/scripts/backup-config.sh

# Hourly memory state backup
0 * * * * cp /opt/lkjagent/data/memory.json /opt/lkjagent/backups/memory-$(date +\%H).json
```

### Disaster Recovery

#### Recovery Procedures

```bash
# 1. Stop the application
docker-compose down

# 2. Restore from backup
tar -xzf /opt/lkjagent/backups/lkjagent-config-20240115_020000.tar.gz -C /

# 3. Verify configuration
./build/lkjagent --validate-config

# 4. Restart application
docker-compose up -d

# 5. Verify operation
docker-compose logs -f lkjagent
```

#### Rollback Strategy

```bash
# Quick rollback script
#!/bin/bash
# rollback.sh

PREVIOUS_VERSION="$1"
if [ -z "$PREVIOUS_VERSION" ]; then
    echo "Usage: $0 <previous_version>"
    exit 1
fi

# Stop current version
docker-compose down

# Restore previous image
docker tag "lkjagent:$PREVIOUS_VERSION" lkjagent:latest

# Restore previous configuration
cp "/opt/lkjagent/backups/config-$PREVIOUS_VERSION.json" /opt/lkjagent/data/config.json

# Restart with previous version
docker-compose up -d

echo "Rollback to version $PREVIOUS_VERSION completed"
```

## Performance Tuning

### Memory Pool Optimization

```c
// Adjust pool sizes based on monitoring data
#define POOL_STRING16_MAXCOUNT    (2097152)  // Double if seeing exhaustion
#define POOL_STRING256_MAXCOUNT   (131072)   // Increase for heavy text processing
#define POOL_STRING4096_MAXCOUNT  (8192)     // Adjust based on content size
#define POOL_OBJECT_MAXCOUNT      (131072)   // Increase for complex structures
```

### LLM Optimization

```json
{
  "llm": {
    "connection_pool": {
      "max_connections": 10,
      "connection_timeout": 30,
      "read_timeout": 60
    },
    "request_batching": {
      "enable": true,
      "batch_size": 5,
      "batch_timeout": 1000
    },
    "caching": {
      "enable": true,
      "cache_size": "100MB",
      "ttl": 3600
    }
  }
}
```

### System Optimization

```bash
# System-level optimizations
echo 'vm.swappiness=1' >> /etc/sysctl.conf
echo 'net.core.rmem_max=16777216' >> /etc/sysctl.conf
echo 'net.core.wmem_max=16777216' >> /etc/sysctl.conf

# Apply settings
sysctl -p
```

## Troubleshooting

### Common Issues

#### Memory Pool Exhaustion

**Symptoms:**
- "Pool exhausted" errors in logs
- Application termination
- Performance degradation

**Diagnosis:**
```bash
# Check pool statistics at shutdown
grep "freelist:" /opt/lkjagent/logs/lkjagent.log

# Monitor memory usage
docker stats lkjagent
```

**Solutions:**
- Increase pool sizes in constants
- Implement memory paging
- Optimize memory usage patterns

#### LLM Communication Failures

**Symptoms:**
- "Failed to send HTTP POST request" errors
- Timeout errors
- Invalid response format

**Diagnosis:**
```bash
# Test LLM endpoint directly
curl -X POST "${LLM_ENDPOINT}" \
  -H "Content-Type: application/json" \
  -d '{"model":"test","messages":[{"role":"user","content":"test"}]}'

# Check network connectivity
nc -zv llm-server 1234
```

**Solutions:**
- Verify LLM endpoint configuration
- Check network connectivity
- Implement retry logic with exponential backoff

#### Configuration Issues

**Symptoms:**
- "Failed to parse config JSON" errors
- Missing configuration values
- Application startup failures

**Diagnosis:**
```bash
# Validate JSON syntax
cat /opt/lkjagent/data/config.json | jq .

# Check file permissions
ls -la /opt/lkjagent/data/

# Validate configuration
./build/lkjagent --validate-config
```

**Solutions:**
- Fix JSON syntax errors
- Ensure all required fields are present
- Verify file permissions and ownership

### Debug Mode Deployment

```bash
# Deploy in debug mode for troubleshooting
docker run -d \
  --name lkjagent-debug \
  -v /opt/lkjagent/data:/app/data:rw \
  -e DEBUG=1 \
  -e LOG_LEVEL=DEBUG \
  lkjagent:1.0.0

# Attach to debug session
docker exec -it lkjagent-debug bash
```

### Log Analysis

```bash
# Common log analysis commands
grep "ERROR" /opt/lkjagent/logs/lkjagent.log
grep "Pool exhausted" /opt/lkjagent/logs/lkjagent.log
grep "HTTP POST" /opt/lkjagent/logs/lkjagent.log | tail -20

# Performance analysis
grep "duration_ms" /opt/lkjagent/logs/lkjagent.log | awk '{print $5}' | sort -n
```

## Production Checklist

### Pre-Deployment

- [ ] Build and test in staging environment
- [ ] Validate all configuration parameters
- [ ] Test LLM endpoint connectivity
- [ ] Verify security configurations
- [ ] Setup monitoring and alerting
- [ ] Prepare backup and recovery procedures
- [ ] Document rollback plan

### Deployment

- [ ] Deploy during maintenance window
- [ ] Monitor application startup
- [ ] Verify health checks pass
- [ ] Test basic functionality
- [ ] Monitor performance metrics
- [ ] Verify logging is working
- [ ] Confirm backup procedures

### Post-Deployment

- [ ] Monitor for 24 hours minimum
- [ ] Review performance metrics
- [ ] Check error rates and patterns
- [ ] Verify data persistence
- [ ] Test disaster recovery procedures
- [ ] Update documentation
- [ ] Schedule regular maintenance
