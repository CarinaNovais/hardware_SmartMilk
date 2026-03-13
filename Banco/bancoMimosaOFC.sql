-- MySQL Workbench Forward Engineering

SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION';

-- -----------------------------------------------------
-- Schema mydb
-- -----------------------------------------------------
-- -----------------------------------------------------
-- Schema mimosa
-- -----------------------------------------------------

-- -----------------------------------------------------
-- Schema mimosa
-- -----------------------------------------------------
CREATE SCHEMA IF NOT EXISTS `mimosa` DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci ;
USE `mimosa` ;

-- -----------------------------------------------------
-- Table `mimosa`.`amonia`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`amonia` (
  `idTanque` INT NOT NULL,
  `idRegiao` INT NOT NULL,
  `amonia` DECIMAL(5,2) NOT NULL,
  `data_hora` DATETIME NULL DEFAULT CURRENT_TIMESTAMP)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`co2`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`co2` (
  `idTanque` INT NOT NULL,
  `idRegiao` INT NOT NULL,
  `co2` DECIMAL(7,2) NOT NULL,
  `data_hora` DATETIME NULL DEFAULT CURRENT_TIMESTAMP)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`id`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`id` (
  `idtanque` INT NOT NULL,
  `idregiao` INT NOT NULL,
  `publicacao` MEDIUMBLOB NULL DEFAULT NULL,
  `motivo` MEDIUMBLOB NULL DEFAULT NULL,
  PRIMARY KEY (`idtanque`, `idregiao`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`usuario`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`usuario` (
  `nome` VARCHAR(100) NULL DEFAULT NULL,
  `senha` INT NULL DEFAULT NULL,
  `idtanque` INT NULL DEFAULT NULL,
  `idregiao` INT NULL DEFAULT NULL,
  `cargo` INT NULL DEFAULT NULL,
  `saldo` FLOAT NULL DEFAULT NULL,
  `litros` FLOAT NULL DEFAULT NULL,
  `contato` VARCHAR(15) NULL DEFAULT NULL,
  `foto` MEDIUMBLOB NULL DEFAULT NULL,
  `id` INT NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `nome` (`nome` ASC) VISIBLE,
  INDEX `fk_usuario_id` (`idtanque` ASC, `idregiao` ASC) VISIBLE,
  CONSTRAINT `fk_usuario_id`
    FOREIGN KEY (`idtanque` , `idregiao`)
    REFERENCES `mimosa`.`id` (`idtanque` , `idregiao`))
ENGINE = InnoDB
AUTO_INCREMENT = 28
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`coleta_tanque`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`coleta_tanque` (
  `produtor` VARCHAR(100) NULL DEFAULT NULL,
  `idTanque` INT NULL DEFAULT NULL,
  `idRegiao` INT NULL DEFAULT NULL,
  `ph` FLOAT NULL DEFAULT NULL,
  `temperatura` FLOAT NULL DEFAULT NULL,
  `nivel` FLOAT NULL DEFAULT NULL,
  `amonia` FLOAT NULL DEFAULT NULL,
  `metano` FLOAT NULL DEFAULT NULL,
  `coletor` VARCHAR(100) NULL DEFAULT NULL,
  `placa` VARCHAR(20) NULL DEFAULT NULL,
  `id` INT NOT NULL AUTO_INCREMENT,
  `condutividade` FLOAT NULL DEFAULT NULL,
  `turbidez` FLOAT NULL DEFAULT NULL,
  `co2` FLOAT NULL DEFAULT NULL,
  `idColetor` INT NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  INDEX `fk_coleta_tanque_usuario` (`idColetor` ASC) VISIBLE,
  CONSTRAINT `fk_coleta_tanque_usuario`
    FOREIGN KEY (`idColetor`)
    REFERENCES `mimosa`.`usuario` (`id`))
ENGINE = InnoDB
AUTO_INCREMENT = 24
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`coletores`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`coletores` (
  `coletor` VARCHAR(100) NULL DEFAULT NULL,
  `placa` VARCHAR(20) NULL DEFAULT NULL)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`condutividade`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`condutividade` (
  `idTanque` INT NOT NULL,
  `idRegiao` INT NOT NULL,
  `condutividade` DECIMAL(7,2) NOT NULL,
  `data_hora` DATETIME NULL DEFAULT CURRENT_TIMESTAMP)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`historico`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`historico` (
  `idtanque` INT NOT NULL,
  `idregiao` INT NOT NULL,
  `datahora` DATETIME NOT NULL,
  PRIMARY KEY (`idtanque`, `idregiao`, `datahora`),
  CONSTRAINT `historico_ibfk_1`
    FOREIGN KEY (`idtanque` , `idregiao`)
    REFERENCES `mimosa`.`id` (`idtanque` , `idregiao`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`historico_deposito_produtor`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`historico_deposito_produtor` (
  `id` INT NOT NULL AUTO_INCREMENT,
  `usuario_id` INT NULL DEFAULT NULL,
  `idTanque` INT NULL DEFAULT NULL,
  `idRegiao` INT NULL DEFAULT NULL,
  `ph` FLOAT NULL DEFAULT NULL,
  `temperatura` FLOAT NULL DEFAULT NULL,
  `nivel` FLOAT NULL DEFAULT NULL,
  `amonia` FLOAT NULL DEFAULT NULL,
  `metano` FLOAT NULL DEFAULT NULL,
  `dataDeposito` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  `condutividade` FLOAT NULL DEFAULT NULL,
  `turbidez` FLOAT NULL DEFAULT NULL,
  `co2` FLOAT NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  INDEX `usuario_id` (`usuario_id` ASC) VISIBLE,
  CONSTRAINT `historico_deposito_produtor_ibfk_1`
    FOREIGN KEY (`usuario_id`)
    REFERENCES `mimosa`.`usuario` (`id`))
ENGINE = InnoDB
AUTO_INCREMENT = 9
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`imagens`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`imagens` (
  `nome` VARCHAR(255) NULL DEFAULT NULL,
  `imagem` LONGBLOB NULL DEFAULT NULL)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`lab_devolutiva`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`lab_devolutiva` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `coleta_id` INT NOT NULL,
  `gordura` DECIMAL(5,3) NULL DEFAULT NULL,
  `proteina` DECIMAL(5,3) NULL DEFAULT NULL,
  `lactose` DECIMAL(5,3) NULL DEFAULT NULL,
  `solidos_totais` DECIMAL(5,3) NULL DEFAULT NULL,
  `solidos_nao_gord` DECIMAL(5,3) NULL DEFAULT NULL,
  `densidade` DECIMAL(6,4) NULL DEFAULT NULL,
  `crioscopia` DECIMAL(6,3) NULL DEFAULT NULL,
  `ph` DECIMAL(4,2) NULL DEFAULT NULL,
  `cbt` INT UNSIGNED NULL DEFAULT NULL,
  `ccs` INT UNSIGNED NULL DEFAULT NULL,
  `patogenos` TEXT NULL DEFAULT NULL,
  `antibioticos_pos` TINYINT(1) NULL DEFAULT NULL,
  `antibioticos_desc` TEXT NULL DEFAULT NULL,
  `residuos_quimicos` TEXT NULL DEFAULT NULL,
  `aflatoxina_m1` DECIMAL(6,3) NULL DEFAULT NULL,
  `estabilidade_alc` TINYINT(1) NULL DEFAULT NULL,
  `indice_acidez` DECIMAL(5,3) NULL DEFAULT NULL,
  `tempo_reduc_azul` INT NULL DEFAULT NULL,
  `valor_litro` DECIMAL(10,4) NULL DEFAULT NULL,
  `laboratorio` VARCHAR(120) NULL DEFAULT NULL,
  `laudo_data` DATETIME NULL DEFAULT NULL,
  `observacoes` TEXT NULL DEFAULT NULL,
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `uq_lab_devolutiva_coleta` (`coleta_id` ASC) VISIBLE,
  CONSTRAINT `fk_lab_devolutiva_coleta`
    FOREIGN KEY (`coleta_id`)
    REFERENCES `mimosa`.`coleta_tanque` (`id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
AUTO_INCREMENT = 9
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`litros_registro`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`litros_registro` (
  `nome` VARCHAR(255) NULL DEFAULT NULL,
  `datahora` DATETIME NULL DEFAULT NULL)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`metano`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`metano` (
  `idTanque` INT NOT NULL,
  `idRegiao` INT NOT NULL,
  `metano` DECIMAL(5,2) NOT NULL,
  `data_hora` DATETIME NULL DEFAULT CURRENT_TIMESTAMP)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`nivel`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`nivel` (
  `idTanque` INT NOT NULL,
  `idRegiao` INT NOT NULL,
  `nivel` DECIMAL(5,2) NOT NULL,
  `data_hora` DATETIME NULL DEFAULT CURRENT_TIMESTAMP)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`ph`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`ph` (
  `idTanque` INT NOT NULL,
  `idRegiao` INT NOT NULL,
  `ph` DECIMAL(5,2) NOT NULL,
  `data_hora` DATETIME NULL DEFAULT CURRENT_TIMESTAMP)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`saldo`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`saldo` (
  `nome` VARCHAR(255) NULL DEFAULT NULL,
  `datahora` DATETIME NULL DEFAULT NULL)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`tanque`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`tanque` (
  `idtanque` INT NULL DEFAULT NULL,
  `idregiao` INT NULL DEFAULT NULL,
  `ph` FLOAT NULL DEFAULT NULL,
  `temp` FLOAT NULL DEFAULT NULL,
  `nivel` FLOAT NULL DEFAULT NULL,
  `amonia` FLOAT NULL DEFAULT NULL,
  `metano` FLOAT NULL DEFAULT NULL,
  `status_tanque` VARCHAR(20) NULL DEFAULT NULL,
  `volume` FLOAT NULL DEFAULT NULL,
  `condutividade` FLOAT NULL DEFAULT NULL,
  `turbidez` FLOAT NULL DEFAULT NULL,
  `co2` FLOAT NULL DEFAULT NULL,
  INDEX `fk_tanque_id` (`idtanque` ASC, `idregiao` ASC) VISIBLE,
  CONSTRAINT `fk_tanque_id`
    FOREIGN KEY (`idtanque` , `idregiao`)
    REFERENCES `mimosa`.`id` (`idtanque` , `idregiao`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`tanque_selecionado`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`tanque_selecionado` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `idregiao` INT NOT NULL,
  `idtanque` INT NOT NULL,
  `produtor_id` INT NULL DEFAULT NULL,
  `nome` VARCHAR(120) NULL DEFAULT NULL,
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `coletor_id` INT NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  INDEX `idx_tanque` (`idtanque` ASC, `idregiao` ASC) VISIBLE)
ENGINE = InnoDB
AUTO_INCREMENT = 5
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`temp`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`temp` (
  `idTanque` INT NOT NULL,
  `idRegiao` INT NOT NULL,
  `temp` DECIMAL(5,2) NOT NULL,
  `data_hora` DATETIME NULL DEFAULT CURRENT_TIMESTAMP)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`turbidez`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`turbidez` (
  `idTanque` INT NOT NULL,
  `idRegiao` INT NOT NULL,
  `turbidez` DECIMAL(7,2) NOT NULL,
  `data_hora` DATETIME NULL DEFAULT CURRENT_TIMESTAMP)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `mimosa`.`vacas`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mimosa`.`vacas` (
  `id` INT NOT NULL AUTO_INCREMENT,
  `nome` VARCHAR(100) NULL DEFAULT NULL,
  `brinco` INT NULL DEFAULT NULL,
  `crias` INT NULL DEFAULT NULL,
  `origem` VARCHAR(100) NULL DEFAULT NULL,
  `estado` VARCHAR(100) NULL DEFAULT NULL,
  `usuario_id` INT NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  INDEX `usuario_id` (`usuario_id` ASC) VISIBLE,
  CONSTRAINT `vacas_ibfk_1`
    FOREIGN KEY (`usuario_id`)
    REFERENCES `mimosa`.`usuario` (`id`))
ENGINE = InnoDB
AUTO_INCREMENT = 30
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;

USE `mimosa`;

DELIMITER $$
USE `mimosa`$$
CREATE
DEFINER=`root`@`localhost`
TRIGGER `mimosa`.`trigger_litros`
AFTER UPDATE ON `mimosa`.`usuario`
FOR EACH ROW
BEGIN
    -- Verifica se o valor de litros foi alterado
    IF NEW.litros != OLD.litros THEN
        -- Salva o nome e a data e hora da alteração
        INSERT INTO litros_registro (nome, datahora)
        VALUES (NEW.nome, NOW());
    END IF;
END$$

USE `mimosa`$$
CREATE
DEFINER=`root`@`localhost`
TRIGGER `mimosa`.`trigger_saldo`
AFTER UPDATE ON `mimosa`.`usuario`
FOR EACH ROW
BEGIN
    IF NEW.saldo = 0 AND OLD.saldo != 0 THEN
        INSERT INTO saldo (nome, datahora)
        VALUES (NEW.nome, NOW());
    END IF;
END$$

USE `mimosa`$$
CREATE
DEFINER=`root`@`localhost`
TRIGGER `mimosa`.`registro`
AFTER UPDATE ON `mimosa`.`tanque`
FOR EACH ROW
BEGIN
    IF NEW.nivel = 0 THEN
        INSERT INTO historico (idtanque, idregiao, datahora)
        VALUES (NEW.idtanque, NEW.idregiao, NOW());
    END IF;
END$$


DELIMITER ;

SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;
