package com.karolbystrek.mqttdatahub.configuration;

import com.karolbystrek.mqttdatahub.service.MqttIngestionService;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.integration.dsl.IntegrationFlow;
import org.springframework.integration.mqtt.inbound.MqttPahoMessageDrivenChannelAdapter;
import org.springframework.integration.mqtt.support.DefaultPahoMessageConverter;

@Configuration
public class MqttConfiguration {

    private static final int COMPLETION_TIMEOUT = 5000;
    private static final int QOS = 1;

    private final String brokerUrl;
    private final String clientId;
    private final String sensorDataTopic;

    public MqttConfiguration(
            @Value("${mqtt.broker.url}") String brokerUrl,
            @Value("${mqtt.client.id}") String clientId,
            @Value("${mqtt.topic.sensor-data}") String sensorDataTopic
    ) {
        this.brokerUrl = brokerUrl;
        this.clientId = clientId;
        this.sensorDataTopic = sensorDataTopic;
    }

    @Bean
    public MqttPahoMessageDrivenChannelAdapter mqttInbound() {
        var adapter = new MqttPahoMessageDrivenChannelAdapter(brokerUrl, clientId, sensorDataTopic);
        adapter.setCompletionTimeout(COMPLETION_TIMEOUT);
        adapter.setConverter(new DefaultPahoMessageConverter());
        adapter.setQos(QOS);
        return adapter;
    }

    @Bean
    public IntegrationFlow mqttInboundFlow(MqttPahoMessageDrivenChannelAdapter adapter, MqttIngestionService ingestionService) {
        return IntegrationFlow.from(adapter)
                .handle(ingestionService, "handleMessage")
                .get();
    }
}
