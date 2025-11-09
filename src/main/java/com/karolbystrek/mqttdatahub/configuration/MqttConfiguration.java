package com.karolbystrek.mqttdatahub.configuration;

import com.karolbystrek.mqttdatahub.service.MqttIngestionService;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.integration.dsl.IntegrationFlow;
import org.springframework.integration.mqtt.core.DefaultMqttPahoClientFactory;
import org.springframework.integration.mqtt.core.MqttPahoClientFactory;
import org.springframework.integration.mqtt.inbound.MqttPahoMessageDrivenChannelAdapter;
import org.springframework.integration.mqtt.support.DefaultPahoMessageConverter;

@Configuration
public class MqttConfiguration {

    private static final int COMPLETION_TIMEOUT = 5000;
    private static final int QOS = 1;

    private final String brokerUrl;
    private final String clientId;
    private final String sensorDataTopic;
    private final String username;
    private final String password;

    public MqttConfiguration(
            @Value("${mqtt.broker.url}") String brokerUrl,
            @Value("${mqtt.client.id}") String clientId,
            @Value("${mqtt.topic.sensor-data}") String sensorDataTopic,
            @Value("${mqtt.username}") String username,
            @Value("${mqtt.password}") String password
    ) {
        this.brokerUrl = brokerUrl;
        this.clientId = clientId;
        this.sensorDataTopic = sensorDataTopic;
        this.username = username;
        this.password = password;
    }

    @Bean
    public MqttConnectOptions mqttConnectOptions() {
        MqttConnectOptions options = new MqttConnectOptions();
        options.setServerURIs(new String[]{brokerUrl});
        options.setUserName(username);
        options.setPassword(password.toCharArray());
        return options;
    }

    @Bean
    public MqttPahoClientFactory mqttClientFactory(MqttConnectOptions mqttConnectOptions) {
        DefaultMqttPahoClientFactory factory = new DefaultMqttPahoClientFactory();
        factory.setConnectionOptions(mqttConnectOptions);
        return factory;
    }

    @Bean
    public MqttPahoMessageDrivenChannelAdapter mqttInbound(MqttPahoClientFactory mqttClientFactory) {
        var adapter = new MqttPahoMessageDrivenChannelAdapter(clientId, mqttClientFactory, sensorDataTopic);
        adapter.setCompletionTimeout(COMPLETION_TIMEOUT);
        adapter.setConverter(new DefaultPahoMessageConverter());
        adapter.setQos(QOS);
        return adapter;
    }

    @Bean
    public IntegrationFlow mqttInboundFlow(MqttPahoMessageDrivenChannelAdapter adapter, MqttIngestionService ingestionService) {
        return IntegrationFlow.from(adapter)
                .handle(ingestionService, "handleSensorData")
                .get();
    }
}
