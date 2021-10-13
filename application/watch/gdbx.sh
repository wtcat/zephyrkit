define rst
  monitor reset
end

define ld
  target remote 192.168.199.175:2331
  monitor reset
  load
end

define crash
  set $esf = $arg0
  set $callee = ((const z_arch_esf_t *)$esf)->extra_info.callee
  set $r0 = ((const z_arch_esf_t *) $esf)->basic.a1
  set $r1 = ((const z_arch_esf_t *) $esf)->basic.a2
  set $r2 = ((const z_arch_esf_t *) $esf)->basic.a3
  set $r3 = ((const z_arch_esf_t *) $esf)->basic.a4
  set $r4 = ((const struct _callee_saved *) $callee)->v1
  set $r5 = ((const struct _callee_saved *) $callee)->v2
  set $r6 = ((const struct _callee_saved *) $callee)->v3
  set $r7 = ((const struct _callee_saved *) $callee)->v4
  set $r8 = ((const struct _callee_saved *) $callee)->v5
  set $r9 = ((const struct _callee_saved *) $callee)->v6
  set $r10 = ((const struct _callee_saved *) $callee)->v7
  set $r11 = ((const struct _callee_saved *) $callee)->v8
  set $psp = ((const struct _callee_saved *) $callee)->psp
  set $r12 = ((const z_arch_esf_t *) $esf)->basic.ip
  set $lr = ((const z_arch_esf_t *) $esf)->basic.lr
  bt
end
