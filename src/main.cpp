#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <limits>
#include <algorithm>
#include <map>
#include <ctime>

const int MAX_ORDERS_PER_STATUS = 20;
const double BASE_PRICE = 5000.0;

// Waktu pencucian berdasarkan
// https://www.flyingfishlaundry.com/id/blog/how-long-does-it-typically-take-to-wash-and-dry-clothes-in-commercial-equipment608
const int TIME_WASH = 50;
const int TIME_DRY = 35;
const int TIME_IRON = 40;

enum ClothesType { BAJU = 1, CELANA, JAKET, SELIMUT, LAINNYA };
enum ServiceType { NORMAL = 1, FAST, EXPRESS };
enum StatusType {
	MENUNGGU = 0,
	DICUCI,
	DIKERINGKAN,
	DISETRIKA,
	SELESAI,
	DIBATALKAN
};

struct Order {
	std::string customerName;
	std::chrono::system_clock::time_point entryTime;
	double weight;
	double finalPrice;
	int id;
	int priority;
	int clothesType;
	int serviceType;
	int status;
	bool isDiscounted;
};

std::vector<Order> orders;
int dashboardMatrix[6][MAX_ORDERS_PER_STATUS];
int orderIdCounter = 1;

std::string toLowerCase(const std::string &str)
{
	std::string lower = str;
	std::transform(lower.begin(), lower.end(), lower.begin(), tolower);
	return lower;
}

void resetInput()
{
	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Pembersih console / cmd tanpa bergantung
// Ke tipe atau jenis sistem operasi
void clrscrn()
{
	std::cout << "\033[2J\033[1;1H";
}

std::string getStatusLabel(int status)
{
	switch (status) {
	case MENUNGGU:
		return "Menunggu";
	case DICUCI:
		return "Dicuci";
	case DIKERINGKAN:
		return "Dikeringkan";
	case DISETRIKA:
		return "Disetrika";
	case SELESAI:
		return "Selesai";
	case DIBATALKAN:
		return "Dibatalkan";
	default:
		return "Error: Tipe status tidak ditemukan";
	}
}

std::string getCurrentDateString()
{
	auto now = std::chrono::system_clock::now();
	std::time_t now_c = std::chrono::system_clock::to_time_t(now);

	char buffer[100];
	if (std::strftime(buffer, sizeof(buffer), "%A, %d %B %Y",
			  std::localtime(&now_c))) {
		return std::string(buffer);
	}

	return "Error: Gagal mendapatkan data tanggal";
}

template <typename T> T getValidInput(const std::string &prompt, T min, T max)
{
	T value;
	while (true) {
		std::cout << prompt;
		if (std::cin >> value) {
			if (value >= min && value <= max) {
				return value;
			} else {
				std::cout << "Error: Input harus antara " << min
					  << " dan " << max << ".\n";
				resetInput();
				continue;
			}
		} else {
			std::cout
				<< "Error: Tipe data salah. Masukkan angka yang valid.\n";
			resetInput();
			continue;
		}
	}
}

std::string getValidString(const std::string &prompt, size_t maxChars)
{
	std::string input;

	resetInput();
	while (true) {
		std::cout << prompt;
		getline(std::cin, input);
		if (!input.empty() && input.length() <= maxChars) {
			return input;
		}
		std::cout << "Error: Tidak boleh kosong & Maks " << maxChars
			  << " karakter.\n";
	}
}

void refreshProcessArray()
{
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < MAX_ORDERS_PER_STATUS; j++) {
			dashboardMatrix[i][j] = 0;
		}
	}

	int counters[6] = {0};
	for (const Order &ord : orders) {
		int st = ord.status;
		if (st >= 0 && st <= 5
		    && counters[st] < MAX_ORDERS_PER_STATUS) {
			dashboardMatrix[st][counters[st]] = ord.id;
			counters[st]++;
		}
	}
}

void showHeader()
{
	clrscrn();
	int total = orders.size();
	int selesai = 0, proses = 0;
	for (const Order &o : orders) {
		if (o.status == SELESAI)
			selesai++;
		else
			proses++;
	}

	std::cout << "=== Laundry Express Pro Management App ===\n"
		  << "Hari: " << getCurrentDateString() << "\n"
		  << "Total Order: " << total << "\n"
		  << "Order selesai: " << selesai << "\n"
		  << "Order dalam proses: " << proses << std::endl;
}

int calculateRemainingTime(int currentStatus, int serviceType)
{
	if (currentStatus >= SELESAI || currentStatus == DIBATALKAN)
		return 0;

	int duration = MENUNGGU;
	if (currentStatus == MENUNGGU)
		duration = MENUNGGU;
	else if (currentStatus == DICUCI)
		duration = TIME_WASH;
	else if (currentStatus == DIKERINGKAN)
		duration = TIME_DRY;
	else if (currentStatus == DISETRIKA)
		duration = TIME_IRON;

	double divider = 1.0;
	if (serviceType == FAST)
		divider = 1.5;
	if (serviceType == EXPRESS)
		divider = 2.0;

	int currentStageTime = static_cast<int>(duration / divider);

	return currentStageTime
	       + calculateRemainingTime(currentStatus + 1, serviceType);
}

bool compEff(const Order *a, const Order *b)
{
	return a->weight > b->weight;
}

bool compType(const Order *a, const Order *b)
{
	return a->clothesType < b->clothesType;
}

bool compPrio(const Order *a, const Order *b)
{
	return a->priority < b->priority;
}

bool compareForPrediction(const Order *a, const Order *b)
{
	int status_a = a->status;
	int status_b = b->status;

	int priority_a = a->priority;
	int priority_b = b->priority;

	if (status_a != status_b)
		return status_a > status_b;

	if (priority_a != priority_b)
		return priority_a < priority_b;

	return a->id < b->id;
}

void menu1_tambahOrder()
{
	clrscrn();
	std::cout << "=== Tambah Order Baru ===\n";
	Order newOrder;

	newOrder.customerName = getValidString("Nama Pelanggan: ", 50);
	newOrder.clothesType = getValidInput<int>(
		"Jenis baju/pakaian:\n1.Baju\n2.Celana\n3.Jaket\n4.Selimut\n5.Lain\nMasukan (Pilih dengan angka 1-5): ",
		1, 5);
	newOrder.weight =
		getValidInput<double>("Berat (0.5 - 20 kg): ", 0.5, 20.0);
	newOrder.serviceType = getValidInput<int>(
		"Layanan (1.Normal 2.Fast 3.Express): ", 1, 3);
	newOrder.priority =
		getValidInput<int>("Prioritas (1.Urgent - 5.Santai): ", 1, 5);

	newOrder.id = orderIdCounter++;
	newOrder.status = MENUNGGU;
	newOrder.entryTime = std::chrono::system_clock::now();
	newOrder.isDiscounted = false;

	double multCloth;
	switch (newOrder.clothesType) {
	case BAJU:
		multCloth = 1.0;
		break;
	case CELANA:
		multCloth = 1.2;
		break;
	case JAKET:
		multCloth = 1.5;
		break;
	case SELIMUT:
		multCloth = 2.0;
		break;
	case LAINNYA:
		multCloth = 1.3;
		break;
	}

	double multSvc;
	switch (newOrder.serviceType) {
	case EXPRESS:
		multSvc = 2.0;
		break;
	case FAST:
		multSvc = 1.5;
		break;
	case NORMAL:
		multSvc = 1.0;
		break;
	}
	double price = newOrder.weight * BASE_PRICE * multCloth * multSvc;

	size_t orderCount;
	std::string keyLower = toLowerCase(newOrder.customerName);
	for (const Order &o : orders) {
		if (toLowerCase(newOrder.customerName).find(keyLower)
		    != std::string::npos) {
			if (orderCount >= 3)
				break;
			orderCount++;
		}
	}

	if (orderCount >= 3) {
		price *= 0.85;
		newOrder.isDiscounted = true;
	} else if (newOrder.weight >= 10.0) {
		price *= 0.9;
		newOrder.isDiscounted = true;
	}

	newOrder.finalPrice = price;
	orders.push_back(newOrder);
	refreshProcessArray();
	std::cout << "Order ID " << newOrder.id << " Berhasil! Biaya: Rp "
		  << static_cast<long>(price) << std::endl;
	resetInput();
}

void menu2_prosesOrder()
{
	clrscrn();
	std::cout << "=== Update / Batalkan Order ===\n";

	if (orders.size() == 0) {
		std::cout
			<< "Error: Belum ada order yang dibuat. Tolong tambahkan order terlebih dahulu"
			<< "\n";
		return;
	}
	refreshProcessArray();

	for (int i = 0; i < 5; i++) {
		std::cout << getStatusLabel(i) << "\t: ";

		for (int j = 0; j < MAX_ORDERS_PER_STATUS; j++) {
			if (dashboardMatrix[i][j] != 0)
				std::cout << "[" << dashboardMatrix[i][j]
					  << "] ";
		}

		std::cout << "\n";
	}

	Order last = orders.back();
	int id = getValidInput<int>("\nMasukkan ID Order: ", 1, last.id);
	bool found = false;

	for (Order &o : orders) {
		if (o.id == id) {
			found = true;
			if (o.status >= SELESAI || o.status == DIBATALKAN) {
				std::cout
					<< "Order sudah selesai/batal. Tidak bisa diubah.\n";
			} else {
				std::cout << "Status saat ini: "
					  << getStatusLabel(o.status) << "\n";

				std::cout
					<< "1. Lanjut ke proses berikutnya\n2. BATALKAN ORDER\n3. Kembali\n";
				int act = getValidInput<int>("Pilih aksi: ", 1,
							     3);

				if (act == 1) {
					std::cout << "Status update ke: "
						  << getStatusLabel(o.status)
						  << "\n";

					o.status++;
				} else if (act == 2) {
					std::cout
						<< "Yakin batalkan order? (y/n): ";

					char sure;
					std::cin >> sure;
					if (sure == 'y' || sure == 'Y') {
						o.status = DIBATALKAN;
						std::cout
							<< "Order DIBATALKAN.\n";
					}
				}
			}
			break;
		}
	}
	if (!found)
		std::cout << "Error: ID tidak ditemukan.\n";

	refreshProcessArray();
	resetInput();
}

void menu3_cariOrder()
{
	clrscrn();
	std::cout << "=== Pencarian Data ===\n"
		  << "1. Cari ID / Nomor Order\n"
		  << "2. Cari Nama\n"
		  << "3. Cari Berdasarkan Status\n";

	int mode = getValidInput<int>("Pilih: ", 1, 3);
	if (mode == 1) {
		int id = getValidInput<int>("Masukkan ID: ", 1, 9999);

		for (const Order &o : orders) {
			if (o.id == id) {
				std::cout << "ID: " << o.id
					  << "\nNama: " << o.customerName
					  << "\nStatus: "
					  << getStatusLabel(o.status) << "\n";

				return;
			}
		}

		std::cout << "Error: Tidak ditemukan.\n";
	} else if (mode == 2) {
		std::string key = getValidString("Masukkan Nama: ", 50);
		std::string keyLower = toLowerCase(key);
		bool found = false;

		for (const Order &o : orders) {
			if (toLowerCase(o.customerName).find(keyLower)
			    != std::string::npos) {

				std::cout << "ID: " << o.id
					  << "\nNama: " << o.customerName
					  << "\nStatus: "
					  << getStatusLabel(o.status) << "\n";

				found = true;
			}
		}

		if (!found)
			std::cout << "Tidak ditemukan.\n";
	} else if (mode == 3) {
		std::cout
			<< "Pilih Status:\n1.Menunggu\n2.Cuci\n3.Kering\n4.Setrika\n5.Selesai\nMasukan (1-5): ";
		// Walaupun pada status type mulai dari 0, penggunaan 0 akan
		// menurunkan kualitas penggunaan program
		int st = getValidInput<int>("Input: ", 1, 5) - 1;
		std::map<std::string, std::vector<int>> groupedOrders;

		for (const Order &o : orders) {
			if (o.status == st) {
				groupedOrders[o.customerName].push_back(o.id);
			}
		}

		if (groupedOrders.empty()) {
			std::cout << "Tidak ada order pada status "
				  << getStatusLabel(st) << "\n";
		} else {
			std::cout << "\nHASIL PENCARIAN STATUS: "
				  << getStatusLabel(st) << " ---\n";
			for (auto const &[name, ids] : groupedOrders) {
				std::cout << "Nama: " << name << "\n"
					  << "Jumlah order: " << ids.size()

					  << "List order: ";
				for (size_t i = 0; i < ids.size(); i++) {
					std::cout
						<< ids[i] << ":"
						<< getStatusLabel(st)
						<< (i == ids.size() - 1 ? ""
									: ", ");
				}
				std::cout << std::endl;
			}
		}
	}

	resetInput();
}

void menu4_estimasi()
{
	clrscrn();
	std::cout << "=== Prediksi Waktu Rekursif ===\n";

	Order last = orders.back();
	int id = getValidInput<int>("Masukkan ID Order: ", 1, last.id);

	const Order *targetOrder = nullptr;
	for (const Order &o : orders) {
		if (o.id == id) {
			targetOrder = &o;
			break;
		}
	}

	if (!targetOrder) {
		std::cout << "Error: Order tidak ditemukan." << std::endl;
		return;
	}

	if (targetOrder->status >= SELESAI
	    || targetOrder->status == DIBATALKAN) {
		std::cout << "Order sudah selesai atau dibatalkan."
			  << std::endl;
		return;
	}

	std::vector<const Order *> queue;
	for (const Order &o : orders) {
		if (o.status < SELESAI && o.status != DIBATALKAN) {
			queue.push_back(&o);
		}
	}

	sort(queue.begin(), queue.end(), compareForPrediction);
	int totalMinutes = 0;

	std::cout
		<< "Posisi dalam antrian pengerjaan (diurutkan Prio & Status):\n";
	for (const Order *o : queue) {
		int myTime = calculateRemainingTime(o->status, o->serviceType);

		totalMinutes += myTime;
		if (o->id == id) {
			std::cout << ">> Order anda (ID " << id << ") <<\n";
			break;
		} else {
			std::cout << "- ID " << o->id << " (Prio "
				  << o->priority << ")\n";
		}
	}

	std::cout << "\nEstimasi Selesai: " << totalMinutes << " menit lagi."
		  << std::endl;
	resetInput();
}

void menu5_laporan()
{
	clrscrn();
	std::cout << "=== Laporan Bisnis Harian ===\n";
	if (orders.empty())
		goto error_handler;

	// Enakpsulasi menggunakan {} untuk menghindari masalah dari dua goto
	// pada fungsi yagn sama
	{
		double totalYield = 0, totalWeight = 0;
		int activeCount = 0, validCount = 0;
		std::map<int, int> svcCount;
		std::map<std::string, int> custOrderCount;

		for (const Order &o : orders) {
			if (o.status == DIBATALKAN)
				continue;

			totalYield += o.finalPrice;
			totalWeight += o.weight;
			svcCount[o.serviceType]++;
			custOrderCount[o.customerName]++;
			validCount++;

			if (o.status < SELESAI)
				activeCount++;
		}

		if (validCount == 0)
			goto error_handler;

		// Buat vector berisi pair / tuple dimana ada string dan int
		std::vector<std::pair<std::string, int>> topCust(
			custOrderCount.begin(), custOrderCount.end());

		// Second pada comparator sort ini menunjukan pada element kedua
		// di pair
		sort(topCust.begin(), topCust.end(),
		     [](const std::pair<std::string, int> &a,
			const std::pair<std::string, int> &b) {
			     return a.second > b.second;
		     });

		int popSvc = 1, maxS = 0;
		for (auto const &[svc, count] : svcCount) {
			if (count > maxS) {
				maxS = count;
				popSvc = svc;
			}
		}

		// Ternary operation yang jika di ubah menjadi if-else adalah
		// if popSvc == 1 else if popSvc == 2 else
		std::string svcName =
			(popSvc == 1) ? "Normal"
				      : (popSvc == 2 ? "Fast" : "Express");

		std::cout << "1. Total Pendapatan      : Rp "
			  << static_cast<long>(totalYield) << "\n"
			  << "2. Rata-rata Berat       : "
			  << (totalWeight / validCount) << " Kg\n"
			  << "3. Layanan Terpopuler    : " << svcName << "\n"
			  << "4. Estimasi Kapasitas    : " << activeCount
			  << " order belum selesai.\n";

		std::cout << "5. Top 5 Pelanggan       :\n";
		for (size_t i = 0; i < topCust.size() && i < 5; i++) {
			std::cout << "   - " << topCust[i].first << " ("
				  << topCust[i].second << " order)\n";
		}
	}

	std::cout << std::endl;
	resetInput();
	return;

error_handler:
	std::cout << ">> Data tidak cukup untuk membuat laporan.\n";
}

void menu6_optimasi()
{
	clrscrn();
	std::cout << "=== Optimisasi Mesin ===\n"
		  << "Metode:\n"
		  << "1. Berdasarkan Prioritas\n"
		  << "2. Berdasarkan Efisiensi\n"
		  << "3. Kelompok Tipe Pakaian\n ";

	int m = getValidInput<int>("Pilih: ", 1, 3);
	std::vector<const Order *> ptrs;
	for (const auto &o : orders) {
		if (o.status == MENUNGGU)
			ptrs.push_back(&o);
	}

	if (ptrs.empty()) {
		std::cout << "Tidak ada order menunggu." << std::endl;
		return;
	}

	if (m == 1)
		sort(ptrs.begin(), ptrs.end(), compPrio);
	else if (m == 2)
		sort(ptrs.begin(), ptrs.end(), compEff);
	else if (m == 3)
		sort(ptrs.begin(), ptrs.end(), compType);

	std::cout << "\nSaran Urutan Pengerjaan:\n";
	for (const auto *p : ptrs) {
		std::cout << "ID: " << p->id << "\n"
			  << "Nama customer: " << p->customerName << "\n"
			  << "Priority: " << p->priority << "\n"
			  << "Berat: " << p->weight << "\n"
			  << "Tipe baju: " << p->clothesType << "\n";
	}

	std::cout << std::endl;
	resetInput();
}

void menu7_analisis()
{
	clrscrn();
	std::cout << "=== Analisis data pelanggan ===" << "\n";
	std::string key = getValidString("Masukkan Nama: ", 50);
	std::string keyLower = toLowerCase(key);
	bool found = false;

	double totalSppending = 0.0;

	std::cout << "Order atas nama: " << key << "\n";
	size_t orderNum = 1;
	for (const Order &o : orders) {
		if (toLowerCase(o.customerName).find(keyLower)
		    != std::string::npos) {

			std::cout << "\nOrder #" << orderNum << "\n"
				  << "ID: " << o.id << "\n"
				  << "Status: " << getStatusLabel(o.status)
				  << "\n"
				  << "Berat: " << o.weight << "\n"
				  << "Harga: " << o.finalPrice << "\n";
			found = true;
			orderNum++;
			totalSppending += o.finalPrice;
		}
	}

	if (totalSppending != 0.0)
		std::cout << "Total spending pelanggan atas nama " << key
			  << " adalah: " << totalSppending << std::endl;

	if (!found)
		std::cout << "Data order pelangan tidak ditemukan..."
			  << std::endl;

	resetInput();
}

void menu8_reset()
{
	clrscrn();
	std::cout << "=== Reset Data ===" << "\n";
	char c;
	std::cout << "Reset Data? (y/n): ";
	std::cin >> c;
	if (c == 'y' || c == 'Y') {
		orders.clear();
		orderIdCounter = 1;
		refreshProcessArray();
		std::cout << "Data Reset." << std::endl;
	}

	resetInput();
}

int main()
{
	while (true) {
		showHeader();
		std::cout << "1. Tambah Order\n"
			  << "2. Proses Order (Update/Batal)\n"
			  << "3. Cari Data\n"
			  << "4. Prediksi Waktu\n"
			  << "5. Laporan Bisnis\n"
			  << "6. Optimasi Mesin\n"
			  << "7. Analisis Data\n"
			  << "8. Reset Data\n"
			  << "9. Keluar\n";

		int choice = getValidInput<int>("Pilih Menu: ", 1, 9);

		switch (choice) {
		case 1:
			menu1_tambahOrder();
			break;
		case 2:
			menu2_prosesOrder();
			break;
		case 3:
			menu3_cariOrder();
			break;
		case 4:
			menu4_estimasi();
			break;
		case 5:
			menu5_laporan();
			break;
		case 6:
			menu6_optimasi();
			break;
		case 7:
			menu7_analisis();
			break;
		case 8:
			menu8_reset();
			break;
		case 9:
			return 0;
		}
		std::cout << "\n[Enter] kembali ke menu...";
		resetInput();
		std::cin.get();
	}
}
