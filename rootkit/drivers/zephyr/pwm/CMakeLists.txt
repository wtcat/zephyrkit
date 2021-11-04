# PWM driver

zephyr_sources_ifdef(CONFIG_PWM_APOLLO3P 
  apollo3p_ctimer.c
  apollo3p_pwm.c
)
