# Parts canvass — WasteWise Bin prototype

## Revised budget — 2026-11-12: PHP 2,000 remaining spend

This replaces the purchase plan below. The October comparison and existing Excel workbook are historical, not the current shopping list. Assumes the user's pictured ESP32 starter kit is already paid for; budget includes shipping and a basic reused-material enclosure. These are spending allowances, not a confirmed checkout quotation.

Reuse: ESP32 development board, Micro-USB cable, 830-point breadboard, existing jumper wires, LED, passive buzzer, push button, 220-ohm and 1k resistors. Exact ESP32 module still needs confirmation before wiring. No spare ESP32, extra display or extra RFID-card pack needed initially.

| Buy / reserve | Quantity and specification | Allowance (PHP) |
|---|---|---:|
| Ultrasonic sensor | 1 HC-SR04 sensor (not bracket-only variant) | 50 |
| Servo | 1 SG90 180-degree positional, with horn, for a lightweight flap | 100 |
| RFID kit | 1 RC522 13.56 MHz reader with compatible demo card/tag | 150 |
| Weight sensing | 2 straight-bar 5kg load cells AND 2 HX711 boards; aim <=200 per complete set | 400 |
| Break beam | 1 split transmitter/receiver pair needed; allow the user's pictured 4-pair pack | 450 |
| Power | Regulated 5V supply rated at least 2A; verify suitable connector/cable | 200 |
| Wiring / assembly extras | Extra jumpers as needed, capacitor, power connector, solder/heat-shrink | 100 |
| Basic enclosure and mounts | Reused bins/wood, rigid weighing platforms, screws/spacers, lightweight flap and intake | 250 |
| Shipping / collection | Combined orders where possible | 150 |
| Planned spend | | **1,850** |
| Unallocated buffer | | **150** |
| Budget | | **2,000** |

### Price evidence and shopping links

- [Circuitrocks 5kg bar](https://circuit.rocks/products/weight-load-cell-5kg-straight-bar-2): product page showed PHP65 each; [HX711](https://circuit.rocks/products/weight-sensor-amplifier-hx711) PHP55 each, but only one amplifier shown available. Two complete sets would be PHP240 only if enough stock is available. Collection/product stock disagreed; do not assume both sets can be ordered. PHP400 is the reserved alternative budget.
- [Circuitrocks SG90](https://circuit.rocks/products/tower-pro-9g-sg-90-mini-servo): PHP89 reference; collection/product stock disagreed.
- [Circuitrocks HC-SR04 listing](https://circuit.rocks/products/ultrasonic-ranging-sensor-hc-sr04-and-bracket-for-distance-sensors-in): multiple sensor/bracket variants; displayed prices cannot establish the selected sensor's checkout cost. PHP50 is an allowance.
- [Fu-labs RC522 kit](https://fulabs.ph/products/rc522-mfrc-52213-56mhz-13-56-mhz-rfid-key-card-reader-module-set-for-arduino): page listed RC522+Card+S50+Pin Header at PHP52; confirm contents and shipping. PHP150 allows alternatives.
- Circuitrocks official storefronts: [Shopee](https://shopee.ph/circuitrocks), [Lazada](https://www.lazada.com.ph/shop/circuitrocks). Website prices are not guaranteed to match marketplace prices.
- User-provided break-beam screenshot: PHP442.56 for a claimed four-pair pack; no product URL supplied, so seller, variant, output circuit and live price remain unverified. Reserve PHP450, but confirm 3.3V-safe receiver output before purchasing.
- Power, consumables, enclosure and shipping figures are planning allowances, not seller quotes. Borrow tools (soldering iron, multimeter, drill); buying those is not included. Reuse rigid materials without compromising load-cell mounting. A new finished enclosure may exceed this budget.

### Keep the full prototype functional

Keep both weight channels. Each compartment needs its own load cell and HX711. The kit's reflective obstacle sensor is not a confirmed replacement for the split break beam. Test the chosen beam with clear PET bottles and calibrate both load cells after mounting. Load-cell capacity includes the platform/container weight. Confirm supply and signal voltages before wiring; no 5V signal directly into ESP32 GPIO.

## Historical October comparison (superseded)

_Canvassed 2026-10-22 from Circuitrocks (circuit.rocks) and Lazada PH search results. Prices change daily, and Lazada listings often show the cheapest **variant** first, so open the listing and confirm the exact variant (e.g. **5 kg** load cell) before checkout. Shipping is not included._

## Comparison

| # | Part (qty) | Circuitrocks | Lazada (range seen) | ✅ Pick | Pick total |
|---|---|---|---|---|---|
| 1 | **ESP32 DevKit V1** 30-pin, CH340, micro-USB/Type-C (×2, one spare) | WROOM-32D/U DevKitC ₱831 (1 left) | ₱239–397 | Lazada "ESP32 ESP32S DEVKIT V1 … CH340 ESP-WROOM-32" ₱239 | ₱478 |
| 2 | **HC-SR04** ultrasonic (×1) | ₱25 + bracket, 46 in stock | ₱35–62 (top seller ₱45, 7.8K sold) | Circuitrocks | ₱25 |
| 3 | **SG90** servo (×1) | ₱79, sold out | ₱44–329 (TowerPro SG90 ₱84, 1.4K sold) | Lazada TowerPro SG90 | ₱84 |
| 4 | **RC522** reader **with card + keyfob** (×1) | No RC522 (only PN532 ₱458+) | ₱42–145 | Lazada "MFRC522 … with S50 IC Card and Keychain Tag" ₱97 (or Aidans Lab, QC, ₱42) | ₱97 |
| 5 | Extra 13.56 MHz cards for demo (×1 pack) | Mifare Classic 1K 10 pcs ₱125 (1 left) | not canvassed | Circuitrocks | ₱125 |
| 6 | **IR break-beam pair** (×1, comes in 4-pair packs) | Adafruit 5 mm ₱563, sold out | ₱383–631 per 4 pairs | Lazada "4 Pairs IR Break Beam Sensor 3MM" ₱383–401 | ₱401 |
| 7 | **5 kg load cell + HX711** (×2 sets) | 5 kg cell ₱65 (15 in stock); HX711 ₱55 sold out; kit ₱234 sold out | ₱59–262 per set (top seller ₱165, 1.3K sold) | Lazada "Straight Bar Load Cell … HX711" ₱165 (choose **5kg**) | ₱330 |
| 8 | **5 V 2 A USB adapter** (×1) | not canvassed | ₱38–137 | A known-brand or 2.5 A "Raspberry Pi" type (~₱137). The ₱40 no-name ones can cause random resets. | ₱137 |
| 9 | Breadboard 400-point (×1) | ₱45, 21 in stock | not canvassed | Circuitrocks | ₱45 |
| 10 | DuPont jumper wires M-M, M-F, F-F (×3 packs) | ₱35 each, 60 in stock | not canvassed | Circuitrocks | ₱105 |
| 11 | Passive buzzer (×1) | ₱35, sold out | not canvassed | Buy in person (e-Gizmo, Alexan, Deeco) | ~₱30 |
| 12 | LED, 220 Ω, 1 kΩ, 2 kΩ resistors, 470 µF cap, push button | not canvassed | not canvassed | Buy in person | ~₱100 |
| | **Electronics subtotal** | | | | **≈ ₱1,960** |
| 13 | Bin body: 2 pails or bin + divider, 3" PVC pipe, plywood/acrylic, screws, hot glue | — | — | Hardware store / Ace / Handyman | ~₱500–1,100 |
| | **Estimated total (before shipping)** | | | | **≈ ₱2,450–3,050** |

## Buying tips
- **Don't buy the cheap "ESP32 Expansion Board / Power Board" listings** (₱78–₱250). They're breakout shields **without** the ESP32 chip. The title must say "Development Board" and show a board with a USB port.
- **IR break-beam:** many cheap "counting module" listings are slotted (U-shaped) sensors that won't fit across an 80 mm pipe. Get **split / through-beam pairs** (separate emitter and receiver LEDs).
- **Load cell:** listings like "1KG 2KG 5KG 10KG 20KG" show the price of the cheapest variant. Select **5KG** and check the price updates.
- **RC522:** get one that includes the white **S50 card + blue keyfob**, so you have 2 demo cards for free.
- **Order everything at once** from as few sellers as possible to save on shipping. Lazada sellers in Bulacan or Metro Manila arrive faster than "Overseas" ones.
- Buy the small parts (buzzer, LED, resistors, capacitor, button) in person in one trip. They're cheap but annoying to ship.

## Sources
- Circuitrocks search: [ESP32](https://circuit.rocks/search?q=esp32), [HC-SR04](https://circuit.rocks/search?q=hc-sr04), [SG90](https://circuit.rocks/search?q=sg90), [RFID](https://circuit.rocks/search?q=rfid), [break beam](https://circuit.rocks/search?q=break+beam), [load cell](https://circuit.rocks/search?q=load+cell+5kg), [HX711](https://circuit.rocks/search?q=hx711), [buzzer](https://circuit.rocks/search?q=passive+buzzer), [breadboard/jumpers](https://circuit.rocks/search?q=jumper+wire+breadboard)
- Lazada PH search: [ESP32 DevKit V1](https://www.lazada.com.ph/catalog/?q=esp32+devkit+v1), [RC522](https://www.lazada.com.ph/catalog/?q=rc522+rfid+module), [IR break beam](https://www.lazada.com.ph/catalog/?q=ir+break+beam+sensor), [load cell + HX711](https://www.lazada.com.ph/catalog/?q=load+cell+5kg+hx711), [SG90](https://www.lazada.com.ph/catalog/?q=sg90+servo), [HC-SR04](https://www.lazada.com.ph/catalog/?q=hc-sr04+ultrasonic), [5V 2A adapter](https://www.lazada.com.ph/catalog/?q=5v+2a+usb+charger+adapter)
