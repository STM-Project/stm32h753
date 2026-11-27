# STM32CubeMX

- Keep application logic in `Application/` or within an existing `USER CODE BEGIN` / `USER CODE END` section of a CubeMX-generated file. Preserve the marker names. Do not put application fixes into the generated portions or patch generated middleware to carry application behavior.
- Keep the application linker in `Application/STM32H753_application.ld`. CubeIDE Debug and Release must use it and keep the local update-key pre-build step. Preserve the bootloader application vector base at `0x08020000`.
- After changes in generated files, run the applicable CubeMX preservation checks and build the firmware. If CubeMX itself is unavailable, report that regeneration was verified only by source checks.
