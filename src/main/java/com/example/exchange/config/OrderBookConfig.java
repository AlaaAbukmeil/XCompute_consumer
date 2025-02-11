/* (C)2024 */
package com.example.exchange.config;

import java.util.HashMap;
import java.util.Map;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import com.example.exchange.service.KafkaProducer;
import com.example.exchange.service.OrderBook;

@Configuration
public class OrderBookConfig {

  private Map<String, OrderBook> orderBooks;
  private final KafkaProducer kafkaProducerService;

  public OrderBookConfig(KafkaProducer kafkaProducerService) {
    this.kafkaProducerService = kafkaProducerService;
  }

  @Bean
  public Map<String, OrderBook> orderBooks() {
    if (orderBooks == null) {
      orderBooks = new HashMap<>();
      String[] symbols = {"AAPL", "GOOGL", "MSFT", "AMZN", "FB"};
      for (String symbol : symbols) {
        OrderBook orderBook = new OrderBook();
        orderBook.setSymbol(symbol);
        orderBooks.put(symbol, orderBook);
      }
    }
    return orderBooks;
  }

  public Map<String, OrderBook> getOrderBooks() {
    return orderBooks();
  }

  public OrderBook getOrderBook(String symbol) {
    return orderBooks.get(symbol);
  }
}
