/* (C)2024 */
package com.example.exchange.controller;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;

import com.example.exchange.service.OrderService;
import com.example.exchange.util.JwtUtil;
import com.fasterxml.jackson.databind.ObjectMapper;

@RestController
public class PlaceOrder {
  private final ObjectMapper objectMapper;
  private final JwtUtil jwtUtil;
  private final OrderService orderService;

  @Autowired
  public PlaceOrder(ObjectMapper objectMapper, JwtUtil jwtUtil, OrderService orderService) {
    this.orderService = orderService;
    this.objectMapper = objectMapper;
    this.jwtUtil = jwtUtil;
  }

  @GetMapping("/ping")
  public ResponseEntity<String> testing() {
    return ResponseEntity.ok("Hello");
  }
}
